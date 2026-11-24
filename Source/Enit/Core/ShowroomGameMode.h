#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShowroomGameMode.generated.h"

/**
 * Remplace BP_XRGameMode.
 * Fait apparaitre un AShowroomPawn pour chaque joueur.
 * (les controles sont crees par le pawn : voir Input/ShowroomInput.cpp)
 */
UCLASS()
class ENIT_API AShowroomGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AShowroomGameMode();
};
