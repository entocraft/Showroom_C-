#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ShowroomSessions.generated.h"

class FOnlineSessionSearch;

/** Une session trouvee sur le reseau. */
USTRUCT(BlueprintType)
struct ENIT_API FShowroomSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Sessions") FString Name;
	UPROPERTY(BlueprintReadOnly, Category = "Sessions") int32 Players = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Sessions") int32 MaxPlayers = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Sessions") int32 PingMs = 0;
	/** A passer a JoinSession. */
	UPROPERTY(BlueprintReadOnly, Category = "Sessions") int32 Index = -1;
};

DECLARE_MULTICAST_DELEGATE(FOnShowroomSessionsChanged);

/**
 * SESSIONS MULTIJOUEUR (heberger / rechercher / rejoindre / quitter), sur le reseau local par defaut.
 *
 * Utilise l'Online Subsystem d'Unreal (Config/DefaultEngine.ini : [OnlineSubsystem] DefaultPlatformService=Null).
 *   - Heberger : cree la session puis relance le niveau en "serveur d'ecoute" (les autres peuvent entrer).
 *   - Rechercher : liste les sessions du reseau local.
 *   - Rejoindre : se connecte a l'hote (meme niveau, avec les autres joueurs).
 *   - Quitter : revient seul dans le niveau.
 * Utilise par le menu de la main (page Sessions). Accessible partout : GetGameInstance()->GetSubsystem<UShowroomSessions>().
 */
UCLASS()
class ENIT_API UShowroomSessions : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void HostSession(int32 MaxPlayers = 8);

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void FindSessions();

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void JoinSession(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void LeaveSession();

	UFUNCTION(BlueprintPure, Category = "Sessions")
	const TArray<FShowroomSessionInfo>& GetFoundSessions() const { return Found; }

	/** On heberge (serveur d'ecoute) ou on est connecte a quelqu'un. */
	UFUNCTION(BlueprintPure, Category = "Sessions")
	bool IsInSession() const;

	UFUNCTION(BlueprintPure, Category = "Sessions")
	bool IsHosting() const;

	UFUNCTION(BlueprintPure, Category = "Sessions")
	bool IsBusy() const { return bBusy; }

	/** Phrase d'etat a afficher ("Vous hebergez...", "Recherche...", "Aucune session trouvee"...). */
	UFUNCTION(BlueprintPure, Category = "Sessions")
	FText GetStatusText() const;

	/** Nombre de joueurs dans la partie en cours (1 hors ligne). */
	UFUNCTION(BlueprintPure, Category = "Sessions")
	int32 GetPlayerCount() const;

	/** Appele a chaque changement (recherche terminee, session creee, erreur...). */
	FOnShowroomSessionsChanged OnChanged;

private:
	enum class EPending : uint8 { None, Host, Join, Leave };

	IOnlineSessionPtr GetSessionInterface() const;
	bool IsLan() const;
	void SetMessage(const FString& Message);
	void DestroyThen(EPending Next);
	void DoHost();
	void DoJoin();
	void ReturnToSoloLevel();

	void HandleCreateComplete(FName SessionName, bool bOk);
	void HandleFindComplete(bool bOk);
	void HandleJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroyComplete(FName SessionName, bool bOk);
	void HandleNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error);

	TSharedPtr<FOnlineSessionSearch> Search;
	TArray<FShowroomSessionInfo> Found;
	FString Message;
	FString JoinedName;
	bool bBusy = false;
	bool bSearchedOnce = false;
	int32 PendingMaxPlayers = 8;
	int32 PendingJoinIndex = -1;
	EPending Pending = EPending::None;

	FDelegateHandle CreateHandle, FindHandle, JoinHandle, DestroyHandle, FailureHandle;
};
