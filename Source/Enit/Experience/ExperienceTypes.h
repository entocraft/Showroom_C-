#pragma once

#include "CoreMinimal.h"
#include "ExperienceTypes.generated.h"

class AActor;
class AExperienceSpot;
class AFloatingScreen;
class USoundBase;

/** Comment on passe a l'etape suivante. */
UENUM(BlueprintType)
enum class EExperienceAdvance : uint8
{
	/** Le joueur clique sur "Suivant" dans la fiche. */
	Button      UMETA(DisplayName = "Bouton Suivant"),
	/** Automatiquement apres "Duration" secondes (le bouton Suivant permet de passer plus vite). */
	Timer       UMETA(DisplayName = "Minuteur"),
	/** Quand le joueur entre dans une des zones lumineuses de l'etape (ou clique sur Suivant). */
	ReachSpot   UMETA(DisplayName = "Atteindre la zone lumineuse"),
	/** Quand le narrateur a fini de parler (sans narration : comme le bouton Suivant). */
	AfterNarration UMETA(DisplayName = "Apres la narration"),
};

/**
 * Un objet du niveau mis en valeur pendant une etape :
 * un eclairage par le dessus, un halo lumineux au sol et une etiquette flottante.
 * L'objet lui-meme n'est jamais modifie (il est deja dans le monde).
 */
USTRUCT(BlueprintType)
struct ENIT_API FExperienceHighlightInfo
{
	GENERATED_BODY()

	/** L'objet a mettre en valeur (pipette : cliquer dessus dans le niveau). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mise en valeur")
	TObjectPtr<AActor> Actor = nullptr;

	/** Nom affiche dans l'etiquette au-dessus de l'objet (vide = pas d'etiquette). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mise en valeur")
	FText Label;

	/** Petit texte sous le nom (optionnel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mise en valeur", meta = (MultiLine = true))
	FText Details;
};

/**
 * Une etape d'une experience.
 * Les objets sont deja dans le niveau : une etape ne fait qu'ajouter de la lumiere, de l'interface et du son
 * (mise en valeur d'objets, zones lumineuses a rejoindre, texte de la fiche, voix du narrateur).
 */
USTRUCT(BlueprintType)
struct ENIT_API FExperienceStep
{
	GENERATED_BODY()

	/** Titre affiche sur la fiche pendant cette etape (vide = garde le titre de la fiche). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etape")
	FText Title;

	/** Texte d'explication affiche sur la fiche pendant cette etape (vide = garde la description de la fiche). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etape", meta = (MultiLine = true))
	FText Description;

	/** Objets mis en valeur a cette etape, les uns apres les autres dans l'ordre de la liste. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etape", meta = (TitleProperty = "Label"))
	TArray<FExperienceHighlightInfo> Highlights;

	/** Temps entre deux mises en valeur de la liste (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etape", meta = (ClampMin = "0"))
	float DelayBetweenHighlights = 0.6f;

	/**
	 * Zones lumineuses allumees pendant cette etape pour guider le joueur.
	 * Celles des etapes precedentes restent faiblement allumees.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etape")
	TArray<TObjectPtr<AExperienceSpot>> Spots;

	/**
	 * Ecrans flottants affiches pendant cette etape (caches le reste du temps) : pour donner plus de details
	 * (textes, images...) que le narrateur ne lit PAS. Poser un "Ecran flottant" dans la zone,
	 * Content = WBP_EcranDetail, remplir Title / Body / Image, puis l'ajouter ici.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etape")
	TArray<TObjectPtr<AFloatingScreen>> Screens;

	/** Placer directement le joueur sur la premiere zone lumineuse de l'etape. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etape")
	bool bTeleportToSpot = false;

	/** Comment on passe a l'etape suivante. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etape")
	EExperienceAdvance Advance = EExperienceAdvance::Button;

	/** Duree de l'etape (s) pour le mode Minuteur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etape", meta = (ClampMin = "0.1", EditCondition = "Advance == EExperienceAdvance::Timer", EditConditionHides))
	float Duration = 5.f;

	/**
	 * Ce que dit le narrateur a cette etape (vide = le titre puis la description).
	 * Sert a fabriquer la voix de synthese : Scripts/GenerateNarration.py.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrateur", meta = (MultiLine = true))
	FText NarrationText;

	/**
	 * La voix du narrateur pour cette etape.
	 * Remplie automatiquement par Scripts/GenerateNarration.py (voix de synthese) ;
	 * remplacable a tout moment par un vrai enregistrement (glisser un son ici).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrateur")
	TObjectPtr<USoundBase> Narration = nullptr;

	/** Son joue au debut de l'etape (optionnel, en plus du son de changement d'etape). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etape")
	TObjectPtr<USoundBase> Sound = nullptr;
};
