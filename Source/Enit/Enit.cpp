#include "Enit.h"
#include "Core/ShowroomGameMode.h"
#include "Input/ShowroomInput.h"
#include "Misc/CoreDelegates.h"
#include "Engine/Engine.h"
#include "GameFramework/GameModeBase.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogShowroom);

/**
 * Module du jeu. Seul ajout : un avertissement si le niveau utilise encore l'ancien GameMode
 * Blueprint (World Settings > GameMode Override), car le code C++ ne serait alors pas utilise.
 */
class FEnitModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// Les controles doivent exister AVANT le demarrage de la session VR (OpenXR les lit a ce moment-la).
		if (GEngine && GEngine->IsInitialized())
		{
			UShowroomInput::RegisterAsDefaultControls();
		}
		else
		{
			FCoreDelegates::GetOnPostEngineInit().AddStatic(&UShowroomInput::RegisterAsDefaultControls);
		}

		Handle = FGameModeEvents::GameModeInitializedEvent.AddLambda([](AGameModeBase* GameMode)
		{
			if (GameMode && !GameMode->IsA<AShowroomGameMode>())
			{
				const FString Msg = FString::Printf(
					TEXT("Showroom : le niveau utilise le GameMode \"%s\" au lieu de ShowroomGameMode -> le code C++ du joueur n'est pas utilise. ")
					TEXT("Corrige-le dans World Settings > GameMode Override."),
					*GameMode->GetClass()->GetName());
				UE_LOG(LogShowroom, Warning, TEXT("%s"), *Msg);
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, Msg);
				}
			}
		});
	}

	virtual void ShutdownModule() override
	{
		FGameModeEvents::GameModeInitializedEvent.Remove(Handle);
	}

private:
	FDelegateHandle Handle;
};

IMPLEMENT_PRIMARY_GAME_MODULE(FEnitModule, Enit, "Enit");
