#include "Core/ShowroomSessions.h"
#include "Enit.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

namespace
{
	const FName ShowroomNameKey(TEXT("SHOWROOM_NAME"));
}

void UShowroomSessions::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (GEngine)
	{
		FailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UShowroomSessions::HandleNetworkFailure);
	}
}

void UShowroomSessions::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(FailureHandle);
	}
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}
	Super::Deinitialize();
}

IOnlineSessionPtr UShowroomSessions::GetSessionInterface() const
{
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	return Subsystem ? Subsystem->GetSessionInterface() : nullptr;
}

bool UShowroomSessions::IsLan() const
{
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	return !Subsystem || Subsystem->GetSubsystemName() == NULL_SUBSYSTEM;
}

void UShowroomSessions::SetMessage(const FString& InMessage)
{
	Message = InMessage;
	UE_LOG(LogShowroom, Log, TEXT("Sessions : %s"), *InMessage);
	OnChanged.Broadcast();
}

bool UShowroomSessions::IsInSession() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Standalone;
}

bool UShowroomSessions::IsHosting() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() == NM_ListenServer;
}

int32 UShowroomSessions::GetPlayerCount() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? FMath::Max(1, GameState->PlayerArray.Num()) : 1;
}

FText UShowroomSessions::GetStatusText() const
{
	if (bBusy)
	{
		return FText::FromString(Message.IsEmpty() ? TEXT("Patientez…") : Message);
	}
	if (IsHosting())
	{
		return FText::FromString(FString::Printf(TEXT("Vous hébergez une session · %d joueur%s"), GetPlayerCount(), GetPlayerCount() > 1 ? TEXT("s") : TEXT("")));
	}
	if (IsInSession())
	{
		return FText::FromString(FString::Printf(TEXT("Connecté à « %s » · %d joueur%s"),
			JoinedName.IsEmpty() ? TEXT("session") : *JoinedName, GetPlayerCount(), GetPlayerCount() > 1 ? TEXT("s") : TEXT("")));
	}
	if (!Message.IsEmpty())
	{
		return FText::FromString(Message);
	}
	return FText::FromString(TEXT("Hors ligne : vous visitez seul le showroom."));
}

// ============================================================================
// Heberger
// ============================================================================

void UShowroomSessions::HostSession(int32 MaxPlayers)
{
	if (bBusy)
	{
		return;
	}
	PendingMaxPlayers = FMath::Clamp(MaxPlayers, 2, 16);
	if (IsInSession())
	{
		DestroyThen(EPending::Host);
	}
	else
	{
		DoHost();
	}
}

void UShowroomSessions::DoHost()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions)
	{
		SetMessage(TEXT("Réseau indisponible (Online Subsystem)."));
		return;
	}
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		DestroyThen(EPending::Host); // une ancienne session traine encore
		return;
	}

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = PendingMaxPlayers;
	Settings.bShouldAdvertise = true;
	Settings.bIsLANMatch = IsLan();
	Settings.bAllowJoinInProgress = true;
	Settings.bUsesPresence = !IsLan();
	Settings.bUseLobbiesIfAvailable = !IsLan();
	Settings.Set(ShowroomNameKey, FString::Printf(TEXT("Showroom de %s"), FPlatformProcess::ComputerName()), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	bBusy = true;
	SetMessage(TEXT("Création de la session…"));
	Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UShowroomSessions::HandleCreateComplete));
	if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
	{
		bBusy = false;
		SetMessage(TEXT("Impossible de créer la session."));
	}
}

void UShowroomSessions::HandleCreateComplete(FName SessionName, bool bOk)
{
	bBusy = false;
	if (!bOk)
	{
		SetMessage(TEXT("Impossible de créer la session."));
		return;
	}
	SetMessage(TEXT("Session créée : les autres peuvent vous rejoindre."));
	// Le niveau est relance en "serveur d'ecoute" : les autres joueurs peuvent s'y connecter.
	UGameplayStatics::OpenLevel(GetWorld(), FName(*UGameplayStatics::GetCurrentLevelName(GetWorld(), true)), true, TEXT("listen"));
}

// ============================================================================
// Rechercher
// ============================================================================

void UShowroomSessions::FindSessions()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (bBusy || !Sessions)
	{
		if (!Sessions) SetMessage(TEXT("Réseau indisponible (Online Subsystem)."));
		return;
	}
	Search = MakeShared<FOnlineSessionSearch>();
	Search->bIsLanQuery = IsLan();
	Search->MaxSearchResults = 20;

	bBusy = true;
	SetMessage(TEXT("Recherche des sessions sur le réseau…"));
	Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UShowroomSessions::HandleFindComplete));
	if (!Sessions->FindSessions(0, Search.ToSharedRef()))
	{
		bBusy = false;
		SetMessage(TEXT("La recherche n'a pas pu démarrer."));
	}
}

void UShowroomSessions::HandleFindComplete(bool bOk)
{
	bBusy = false;
	bSearchedOnce = true;
	Found.Reset();
	if (bOk && Search.IsValid())
	{
		for (int32 i = 0; i < Search->SearchResults.Num(); ++i)
		{
			const FOnlineSessionSearchResult& Result = Search->SearchResults[i];
			FShowroomSessionInfo Info;
			if (!Result.Session.SessionSettings.Get(ShowroomNameKey, Info.Name) || Info.Name.IsEmpty())
			{
				Info.Name = Result.Session.OwningUserName;
			}
			Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
			Info.Players = FMath::Clamp(Info.MaxPlayers - Result.Session.NumOpenPublicConnections, 1, FMath::Max(Info.MaxPlayers, 1));
			Info.PingMs = Result.PingInMs;
			Info.Index = i;
			Found.Add(Info);
		}
	}
	SetMessage(!bOk ? TEXT("La recherche a échoué.")
		: Found.Num() == 0 ? TEXT("Aucune session trouvée sur le réseau local.")
		: FString::Printf(TEXT("%d session%s trouvée%s : visez-en une pour la rejoindre."), Found.Num(), Found.Num() > 1 ? TEXT("s") : TEXT(""), Found.Num() > 1 ? TEXT("s") : TEXT("")));
}

// ============================================================================
// Rejoindre
// ============================================================================

void UShowroomSessions::JoinSession(int32 Index)
{
	if (bBusy || !Search.IsValid() || !Search->SearchResults.IsValidIndex(Index))
	{
		return;
	}
	PendingJoinIndex = Index;
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (IsInSession() || (Sessions && Sessions->GetNamedSession(NAME_GameSession)))
	{
		DestroyThen(EPending::Join);
	}
	else
	{
		DoJoin();
	}
}

void UShowroomSessions::DoJoin()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions || !Search.IsValid() || !Search->SearchResults.IsValidIndex(PendingJoinIndex))
	{
		SetMessage(TEXT("Cette session n'est plus disponible."));
		return;
	}
	const FShowroomSessionInfo* Info = Found.FindByPredicate([this](const FShowroomSessionInfo& I) { return I.Index == PendingJoinIndex; });
	JoinedName = Info ? Info->Name : FString();

	bBusy = true;
	SetMessage(FString::Printf(TEXT("Connexion à « %s »…"), *JoinedName));
	Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UShowroomSessions::HandleJoinComplete));
	if (!Sessions->JoinSession(0, NAME_GameSession, Search->SearchResults[PendingJoinIndex]))
	{
		bBusy = false;
		SetMessage(TEXT("Impossible de rejoindre cette session."));
	}
}

void UShowroomSessions::HandleJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bBusy = false;
	IOnlineSessionPtr Sessions = GetSessionInterface();
	FString Url;
	if (Result != EOnJoinSessionCompleteResult::Success || !Sessions || !Sessions->GetResolvedConnectString(SessionName, Url))
	{
		SetMessage(TEXT("Impossible de rejoindre cette session (pleine ou fermée ?)."));
		return;
	}
	SetMessage(FString::Printf(TEXT("Connexion à « %s »…"), *JoinedName));
	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController(GetWorld()))
	{
		PC->ClientTravel(Url, TRAVEL_Absolute);
	}
}

// ============================================================================
// Quitter
// ============================================================================

void UShowroomSessions::LeaveSession()
{
	if (bBusy)
	{
		return;
	}
	DestroyThen(EPending::Leave);
}

void UShowroomSessions::DestroyThen(EPending Next)
{
	Pending = Next;
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions || !Sessions->GetNamedSession(NAME_GameSession))
	{
		HandleDestroyComplete(NAME_GameSession, true); // rien a detruire
		return;
	}
	bBusy = true;
	SetMessage(TEXT("Fermeture de la session…"));
	Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UShowroomSessions::HandleDestroyComplete));
	if (!Sessions->DestroySession(NAME_GameSession))
	{
		HandleDestroyComplete(NAME_GameSession, false);
	}
}

void UShowroomSessions::HandleDestroyComplete(FName SessionName, bool bOk)
{
	bBusy = false;
	const EPending Next = Pending;
	Pending = EPending::None;
	switch (Next)
	{
	case EPending::Host:
		DoHost();
		break;
	case EPending::Join:
		DoJoin();
		break;
	case EPending::Leave:
		JoinedName.Reset();
		SetMessage(TEXT("Vous avez quitté la session."));
		ReturnToSoloLevel();
		break;
	default:
		break;
	}
}

void UShowroomSessions::ReturnToSoloLevel()
{
	if (IsInSession())
	{
		// Relance le niveau seul (deconnecte les autres si on hebergeait).
		UGameplayStatics::OpenLevel(GetWorld(), FName(*UGameplayStatics::GetCurrentLevelName(GetWorld(), true)));
	}
}

void UShowroomSessions::HandleNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error)
{
	UE_LOG(LogShowroom, Warning, TEXT("Sessions : erreur reseau (%s)"), *Error);
	JoinedName.Reset();
	if (IOnlineSessionPtr Sessions = GetSessionInterface(); Sessions && Sessions->GetNamedSession(NAME_GameSession))
	{
		Sessions->DestroySession(NAME_GameSession);
	}
	SetMessage(TEXT("Connexion perdue : retour en solo."));
}
