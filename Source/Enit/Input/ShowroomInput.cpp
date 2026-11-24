#include "Input/ShowroomInput.h"
#include "Enit.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "EnhancedInputDeveloperSettings.h"
#include "UObject/Package.h"

namespace
{
	// Prefixes des manettes qui ont des noms de touches identiques (Grip_Axis, Trigger_Axis, Thumbstick_2D...).
	const TCHAR* const StandardControllers[] = { TEXT("OculusTouch"), TEXT("ValveIndex"), TEXT("PICO") };

	const TCHAR* SideName(bool bLeft) { return bLeft ? TEXT("Left") : TEXT("Right"); }
}

UShowroomInput* UShowroomInput::Get()
{
	static UShowroomInput* Instance = nullptr;
	if (!Instance)
	{
		Instance = NewObject<UShowroomInput>(GetTransientPackage(), TEXT("ShowroomInput"));
		Instance->AddToRoot(); // jamais detruit
		Instance->Build();
	}
	return Instance;
}

void UShowroomInput::RegisterAsDefaultControls()
{
	UInputMappingContext* Context = Get()->Context;

	// OpenXR (et le joueur local) lisent la liste "Default Mapping Contexts" d'Enhanced Input.
	// On y ajoute notre contexte en memoire (rien n'est ecrit dans les fichiers .ini).
	UEnhancedInputDeveloperSettings* Settings = GetMutableDefault<UEnhancedInputDeveloperSettings>();
	Settings->bEnableDefaultMappingContexts = true;
	Settings->DefaultMappingContexts.RemoveAll([Context](const FDefaultContextSetting& Entry)
	{
		return Entry.InputMappingContext.ToSoftObjectPath() == FSoftObjectPath(Context);
	});
	FDefaultContextSetting Entry;
	Entry.InputMappingContext = Context;
	Entry.Priority = 0;
	Entry.bAddImmediately = true;
	Entry.bRegisterWithUserSettings = false;
	Settings->DefaultMappingContexts.Add(Entry);

	UE_LOG(LogShowroom, Log, TEXT("Controles VR declares : %s (%d touches, %d contexte(s) par defaut)"),
		*Context->GetPathName(), Context->GetMappings().Num(), Settings->DefaultMappingContexts.Num());
}

void UShowroomInput::Build()
{
	Context = NewObject<UInputMappingContext>(this, TEXT("IMC_Showroom"));
	Context->ContextDescription = FText::FromString(TEXT("Showroom"));

	// ------------------------------------------------------------------
	// Petits outils
	// ------------------------------------------------------------------

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		Action->ActionDescription = FText::FromString(Name); // nom visible par OpenXR (doit etre unique)
		return Action;
	};

	// Associe une touche (par son nom Unreal, ex "OculusTouch_Left_Grip_Axis") a une action.
	auto Map = [this](UInputAction* Action, const FString& KeyName) -> FEnhancedActionKeyMapping&
	{
		return Context->MapKey(Action, FKey(*KeyName));
	};

	// Une gachette / un grip (0..1) qui se comporte comme un bouton : "appuye" au-dela de Threshold.
	auto MapAsButton = [this, &Map](UInputAction* Action, const FString& KeyName, float Threshold = 0.5f)
	{
		FEnhancedActionKeyMapping& Mapping = Map(Action, KeyName);
		UInputTriggerDown* Down = NewObject<UInputTriggerDown>(Context);
		Down->ActuationThreshold = Threshold;
		Mapping.Triggers.Add(Down);
	};

	// Un joystick avec une zone morte (evite les derives quand on ne touche a rien).
	auto MapStick = [this, &Map](UInputAction* Action, const FString& KeyName, float DeadZone = 0.2f)
	{
		FEnhancedActionKeyMapping& Mapping = Map(Action, KeyName);
		UInputModifierDeadZone* Zone = NewObject<UInputModifierDeadZone>(Context);
		Zone->LowerThreshold = DeadZone;
		Mapping.Modifiers.Add(Zone);
	};

	// Nom de touche "standard" : <Manette>_<Left|Right>_<Suffixe>
	auto Key = [](const TCHAR* Controller, bool bLeft, const TCHAR* Suffix)
	{
		return FString::Printf(TEXT("%s_%s_%s"), Controller, SideName(bLeft), Suffix);
	};

	// ------------------------------------------------------------------
	// Actions
	// ------------------------------------------------------------------

	Walk            = MakeAction(TEXT("IA_Walk"),             EInputActionValueType::Axis2D);
	Turn            = MakeAction(TEXT("IA_Turn"),             EInputActionValueType::Axis1D);
	Teleport        = MakeAction(TEXT("IA_Teleport"),         EInputActionValueType::Boolean);
	GrabLeft        = MakeAction(TEXT("IA_Grab_Left"),        EInputActionValueType::Boolean);
	GrabRight       = MakeAction(TEXT("IA_Grab_Right"),       EInputActionValueType::Boolean);
	MenuToggleLeft  = MakeAction(TEXT("IA_MenuToggle_Left"),  EInputActionValueType::Boolean);
	MenuToggleRight = MakeAction(TEXT("IA_MenuToggle_Right"), EInputActionValueType::Boolean);
	MenuClickLeft   = MakeAction(TEXT("IA_MenuClick_Left"),   EInputActionValueType::Boolean);
	MenuClickRight  = MakeAction(TEXT("IA_MenuClick_Right"),  EInputActionValueType::Boolean);
	GraspLeft       = MakeAction(TEXT("IA_Grasp_Left"),       EInputActionValueType::Axis1D);
	GraspRight      = MakeAction(TEXT("IA_Grasp_Right"),      EInputActionValueType::Axis1D);
	IndexCurlLeft   = MakeAction(TEXT("IA_IndexCurl_Left"),   EInputActionValueType::Axis1D);
	IndexCurlRight  = MakeAction(TEXT("IA_IndexCurl_Right"),  EInputActionValueType::Axis1D);
	IndexTouchLeft  = MakeAction(TEXT("IA_IndexTouch_Left"),  EInputActionValueType::Boolean);
	IndexTouchRight = MakeAction(TEXT("IA_IndexTouch_Right"), EInputActionValueType::Boolean);
	ThumbTouchLeft  = MakeAction(TEXT("IA_ThumbTouch_Left"),  EInputActionValueType::Boolean);
	ThumbTouchRight = MakeAction(TEXT("IA_ThumbTouch_Right"), EInputActionValueType::Boolean);

	// ------------------------------------------------------------------
	// Touches : Quest / Index / Pico (memes noms)
	// ------------------------------------------------------------------

	for (const TCHAR* C : StandardControllers)
	{
		MapStick(Walk, Key(C, true, TEXT("Thumbstick_2D")));          // marcher : joystick gauche
		MapStick(Turn, Key(C, false, TEXT("Thumbstick_X")), 0.1f);    // tourner : joystick droit
		MapAsButton(Teleport, Key(C, false, TEXT("Trigger_Axis")));  // teleporter : gachette droite

		for (const bool bLeft : { true, false })
		{
			MapAsButton(bLeft ? GrabLeft : GrabRight,           Key(C, bLeft, TEXT("Grip_Axis")));
			MapAsButton(bLeft ? MenuClickLeft : MenuClickRight, Key(C, bLeft, TEXT("Trigger_Axis")));
			Map(bLeft ? GraspLeft : GraspRight,                 Key(C, bLeft, TEXT("Grip_Axis")));
			Map(bLeft ? IndexCurlLeft : IndexCurlRight,         Key(C, bLeft, TEXT("Trigger_Axis")));
			Map(bLeft ? IndexTouchLeft : IndexTouchRight,       Key(C, bLeft, TEXT("Trigger_Touch")));
			Map(bLeft ? ThumbTouchLeft : ThumbTouchRight,       Key(C, bLeft, TEXT("Thumbstick_Touch")));
		}
	}

	// Ouvrir le menu de main : bouton Menu / Y a gauche, B a droite.
	for (const TCHAR* C : { TEXT("OculusTouch"), TEXT("PICO") })
	{
		Map(MenuToggleLeft,  Key(C, true,  TEXT("Menu_Click")));
		Map(MenuToggleLeft,  Key(C, true,  TEXT("Y_Click")));
		Map(MenuToggleRight, Key(C, false, TEXT("B_Click")));

		// Pouce pose sur un bouton
		for (const TCHAR* Button : { TEXT("X_Touch"), TEXT("Y_Touch") }) Map(ThumbTouchLeft,  Key(C, true,  Button));
		for (const TCHAR* Button : { TEXT("A_Touch"), TEXT("B_Touch") }) Map(ThumbTouchRight, Key(C, false, Button));
	}
	Map(MenuToggleLeft,  TEXT("ValveIndex_Left_B_Click"));
	Map(MenuToggleRight, TEXT("ValveIndex_Right_B_Click"));
	for (const bool bLeft : { true, false })
	{
		for (const TCHAR* Button : { TEXT("A_Touch"), TEXT("B_Touch"), TEXT("Trackpad_Touch") })
		{
			Map(bLeft ? ThumbTouchLeft : ThumbTouchRight, Key(TEXT("ValveIndex"), bLeft, Button));
		}
	}

	// ------------------------------------------------------------------
	// Touches : HTC Vive (pave tactile au lieu d'un joystick)
	// ------------------------------------------------------------------

	MapStick(Walk, TEXT("Vive_Left_Trackpad_2D"));
	MapStick(Turn, TEXT("Vive_Right_Trackpad_X"), 0.1f);
	MapAsButton(Teleport, TEXT("Vive_Right_Trigger_Axis"));
	for (const bool bLeft : { true, false })
	{
		Map(bLeft ? GrabLeft : GrabRight,                   Key(TEXT("Vive"), bLeft, TEXT("Grip_Click")));
		Map(bLeft ? MenuToggleLeft : MenuToggleRight,       Key(TEXT("Vive"), bLeft, TEXT("Menu_Click")));
		MapAsButton(bLeft ? MenuClickLeft : MenuClickRight, Key(TEXT("Vive"), bLeft, TEXT("Trigger_Axis")));
		Map(bLeft ? IndexCurlLeft : IndexCurlRight,         Key(TEXT("Vive"), bLeft, TEXT("Trigger_Axis")));
		Map(bLeft ? ThumbTouchLeft : ThumbTouchRight,       Key(TEXT("Vive"), bLeft, TEXT("Trackpad_Touch")));
	}
}
