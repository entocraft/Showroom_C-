#include "Core/ShowroomGameMode.h"
#include "Player/ShowroomPawn.h"

AShowroomGameMode::AShowroomGameMode()
{
	DefaultPawnClass = AShowroomPawn::StaticClass();
}
