#include "Core/ShowroomAudio.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UObject/Package.h"

namespace
{
	/** Attenuation par defaut des sons places dans le monde (si le son n'en a pas lui-meme). */
	USoundAttenuation* GetWorldAttenuation()
	{
		static USoundAttenuation* Attenuation = nullptr;
		if (!Attenuation)
		{
			Attenuation = NewObject<USoundAttenuation>(GetTransientPackage(), TEXT("ShowroomWorldAttenuation"));
			Attenuation->AddToRoot();
			Attenuation->Attenuation.bAttenuate = true;
			Attenuation->Attenuation.bSpatialize = true;
			Attenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
			Attenuation->Attenuation.AttenuationShapeExtents = FVector(300.f, 0.f, 0.f); // plein volume jusqu'a 3 m
			Attenuation->Attenuation.FalloffDistance = 2000.f;
		}
		return Attenuation;
	}
}

void ShowroomAudio::Play2D(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, float Volume)
{
	if (USoundBase* Loaded = Sound.LoadSynchronous())
	{
		UGameplayStatics::PlaySound2D(WorldContext, Loaded, Volume);
	}
}

void ShowroomAudio::PlayAt(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, const FVector& Location, float Volume)
{
	if (USoundBase* Loaded = Sound.LoadSynchronous())
	{
		USoundAttenuation* Attenuation = Loaded->AttenuationSettings ? nullptr : GetWorldAttenuation();
		UGameplayStatics::PlaySoundAtLocation(WorldContext, Loaded, Location, FRotator::ZeroRotator, Volume, 1.f, 0.f, Attenuation);
	}
}

UAudioComponent* ShowroomAudio::Spawn2D(const UObject* WorldContext, USoundBase* Sound, float Volume, float FadeInTime)
{
	if (!Sound)
	{
		return nullptr;
	}
	UAudioComponent* Comp = UGameplayStatics::CreateSound2D(WorldContext, Sound, Volume, 1.f, 0.f, nullptr, /*bPersistAcrossLevelTransition*/ false, /*bAutoDestroy*/ true);
	if (Comp)
	{
		if (FadeInTime > 0.f)
		{
			Comp->FadeIn(FadeInTime, Volume);
		}
		else
		{
			Comp->Play();
		}
	}
	return Comp;
}
