#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ShowroomInput.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * TOUS LES CONTROLES DU JEU, ECRITS EN C++ (plus aucun asset IMC / IA a ouvrir).
 *
 * Les actions et les touches sont creees UNE fois au lancement du jeu (Build(), ShowroomInput.cpp),
 * puis declarees comme contexte par defaut d'Enhanced Input : OpenXR les lit au demarrage
 * de la session VR (sinon le casque ne transmet aucun bouton).
 * Pour changer un bouton : modifie la table correspondante dans ShowroomInput.cpp.
 *
 * Manettes gerees : Meta Quest (OculusTouch), Valve Index, Pico, HTC Vive.
 */
UCLASS()
class ENIT_API UShowroomInput : public UObject
{
	GENERATED_BODY()

public:
	/** L'instance unique (creee a la premiere demande, jamais detruite). */
	static UShowroomInput* Get();

	/** Cree les controles et les declare a Enhanced Input / OpenXR. Appele au demarrage du module. */
	static void RegisterAsDefaultControls();

	/** Le contexte a ajouter au joueur local. */
	UPROPERTY() TObjectPtr<UInputMappingContext> Context;

	// --- Deplacement ---
	UPROPERTY() TObjectPtr<UInputAction> Walk;      // Axis2D : joystick gauche
	UPROPERTY() TObjectPtr<UInputAction> Turn;      // Axis1D : joystick droit gauche/droite
	UPROPERTY() TObjectPtr<UInputAction> Teleport;  // Bool   : gachette droite (maintenir puis relacher)

	// --- Attraper (Started = appui, Completed = relache) ---
	UPROPERTY() TObjectPtr<UInputAction> GrabLeft;
	UPROPERTY() TObjectPtr<UInputAction> GrabRight;

	// --- Menus ---
	UPROPERTY() TObjectPtr<UInputAction> MenuToggleLeft;
	UPROPERTY() TObjectPtr<UInputAction> MenuToggleRight;
	UPROPERTY() TObjectPtr<UInputAction> MenuClickLeft;   // Started = clic enfonce, Completed = relache
	UPROPERTY() TObjectPtr<UInputAction> MenuClickRight;

	// --- Doigts ---
	UPROPERTY() TObjectPtr<UInputAction> GraspLeft;          // Axis1D : grip
	UPROPERTY() TObjectPtr<UInputAction> GraspRight;
	UPROPERTY() TObjectPtr<UInputAction> IndexCurlLeft;      // Axis1D : gachette
	UPROPERTY() TObjectPtr<UInputAction> IndexCurlRight;
	UPROPERTY() TObjectPtr<UInputAction> IndexTouchLeft;     // Bool : doigt pose sur la gachette
	UPROPERTY() TObjectPtr<UInputAction> IndexTouchRight;
	UPROPERTY() TObjectPtr<UInputAction> ThumbTouchLeft;     // Bool : pouce pose sur un bouton / le joystick
	UPROPERTY() TObjectPtr<UInputAction> ThumbTouchRight;

private:
	void Build();
};
