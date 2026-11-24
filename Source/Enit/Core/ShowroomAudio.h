#pragma once

#include "CoreMinimal.h"

class USoundBase;
class UAudioComponent;

/**
 * Petits raccourcis pour jouer les sons du showroom.
 * Les sons eux-memes se choisissent dans Project Settings > Game > Showroom > Sons.
 */
namespace ShowroomAudio
{
	/** Joue un son "dans la tete" (interface, musique...). Sans effet si le son est vide. */
	ENIT_API void Play2D(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, float Volume = 1.f);

	/** Joue un son a une position du monde (on l'entend venir de la). */
	ENIT_API void PlayAt(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, const FVector& Location, float Volume = 1.f);

	/** Lance un son 2D et renvoie son composant (pour l'arreter / changer son volume). */
	ENIT_API UAudioComponent* Spawn2D(const UObject* WorldContext, USoundBase* Sound, float Volume, float FadeInTime = 0.f);
}
