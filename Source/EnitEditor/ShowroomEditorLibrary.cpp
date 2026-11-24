#include "ShowroomEditorLibrary.h"
#include "UI/ShowroomWidget.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "IAssetTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/Package.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "UI/ExperienceSelectorWidget.h"
#include "UI/InfoPanelWidget.h"
#include "Interaction/DetectionZone.h"
#include "ContentStreaming.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"

namespace
{
	/** Vide l'arbre de widgets d'un Widget Blueprint (les anciens widgets partent a la poubelle). */
	void ClearTree(UWidgetBlueprint* Blueprint)
	{
		TArray<UWidget*> Old;
		Blueprint->WidgetTree->GetAllWidgets(Old);
		for (UWidget* Widget : Old)
		{
			Blueprint->OnVariableRemoved(Widget->GetFName());
			Widget->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		}
		Blueprint->WidgetTree->RootWidget = nullptr;
	}
}

UObject* UShowroomEditorLibrary::CreateWidgetBlueprintFromLayout(const FString& PackagePath, const FString& AssetName,
	TSubclassOf<UUserWidget> ParentClass, bool bOverwrite)
{
	if (!ParentClass || !ParentClass->IsChildOf(UShowroomWidget::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("CreateWidgetBlueprintFromLayout : %s n'est pas une classe ShowroomWidget."), *GetNameSafe(ParentClass));
		return nullptr;
	}

	const FString ObjectPath = PackagePath / AssetName + TEXT(".") + AssetName;
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Blueprint && !bOverwrite)
	{
		return Blueprint; // deja la : on ne touche pas aux modifications faites dans l'editeur
	}

	if (!Blueprint)
	{
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->BlueprintType = BPTYPE_Normal;
		Factory->ParentClass = ParentClass;
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		Blueprint = Cast<UWidgetBlueprint>(AssetTools.CreateAsset(AssetName, PackagePath, UWidgetBlueprint::StaticClass(), Factory));
		if (!Blueprint)
		{
			return nullptr;
		}
	}

	// La mise en page par defaut de la classe, construite directement dans le Widget Blueprint.
	ClearTree(Blueprint);
	UShowroomWidget* Builder = NewObject<UShowroomWidget>(GetTransientPackage(), ParentClass);
	Builder->BuildLayoutInto(Blueprint->WidgetTree);

	// Les widgets attendus par le code (meme nom qu'une propriete de la classe) deviennent des variables.
	TArray<UWidget*> Widgets;
	Blueprint->WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Widget : Widgets)
	{
		Widget->SetFlags(RF_Transactional);
		Widget->bIsVariable = FindFProperty<FObjectPropertyBase>(ParentClass, Widget->GetFName()) != nullptr;
		Blueprint->OnVariableAdded(Widget->GetFName());
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	Blueprint->MarkPackageDirty();
	return Blueprint;
}

bool UShowroomEditorLibrary::RenderWidgetPreview(TSubclassOf<UUserWidget> WidgetClass, FVector2D Size, const FString& FilePath, int32 Page)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World || !WidgetClass || Size.X < 1.f || Size.Y < 1.f)
	{
		UE_LOG(LogTemp, Error, TEXT("RenderWidgetPreview : monde, classe ou taille invalide."));
		return false;
	}

	// La 1re zone d'experience du niveau (meme ordre que l'ecran d'accueil).
	ADetectionZone* FirstZone = nullptr;
	for (TActorIterator<ADetectionZone> It(World); It; ++It)
	{
		if (It->bHasExperience && It->bShowInSelector
			&& (!FirstZone || It->SelectorOrder < FirstZone->SelectorOrder
				|| (It->SelectorOrder == FirstZone->SelectorOrder && It->Info.Title.ToString() < FirstZone->Info.Title.ToString())))
		{
			FirstZone = *It;
		}
	}

	UUserWidget* Widget = CreateWidget<UUserWidget>(World, WidgetClass);
	if (!Widget)
	{
		return false;
	}
	if (UInfoPanelWidget* Panel = Cast<UInfoPanelWidget>(Widget))
	{
		if (FirstZone)
		{
			Panel->SetInfo(FirstZone->Info);
			Panel->SetActionButton(true, FirstZone->StartButtonText);
		}
	}

	TSharedRef<SWidget> SlateWidget = Widget->TakeWidget(); // construit le widget (NativeConstruct)

	if (UExperienceSelectorWidget* Hub = Cast<UExperienceSelectorWidget>(Widget))
	{
		if (Page == static_cast<int32>(EShowroomHubPage::Sheet))
		{
			Hub->OpenSheet(FirstZone);
		}
		else
		{
			Hub->ShowPage(static_cast<EShowroomHubPage>(FMath::Clamp(Page, 0, 4)));
		}
	}

	// Images en pleine resolution, puis plusieurs passes de dessin (la mise en page se stabilise).
	IStreamingManager::Get().StreamAllResources(10.f);
	FWidgetRenderer Renderer(/*bUseGammaCorrection*/ true, /*bClearTarget*/ true);
	UTextureRenderTarget2D* Target = Renderer.DrawWidget(SlateWidget, Size);
	for (int32 Pass = 0; Pass < 4 && Target; ++Pass)
	{
		Renderer.DrawWidget(Target, SlateWidget, Size, 0.016f);
	}
	FlushRenderingCommands();
	if (!Target)
	{
		return false;
	}

	IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);
	TUniquePtr<FArchive> File(IFileManager::Get().CreateFileWriter(*FilePath));
	const bool bOk = File && FImageUtils::ExportRenderTarget2DAsPNG(Target, *File);
	UE_LOG(LogTemp, Display, TEXT("[Apercu] %s -> %s (%s)"), *GetNameSafe(WidgetClass), *FilePath, bOk ? TEXT("ok") : TEXT("ECHEC"));
	return bOk;
}

TArray<FString> UShowroomEditorLibrary::DescribeWidgetBlueprint(UObject* WidgetBlueprint)
{
	TArray<FString> Result;
	const UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(WidgetBlueprint);
	if (!Blueprint || !Blueprint->WidgetTree)
	{
		return Result;
	}
	Result.Add(FString::Printf(TEXT("parent=%s statut=%d"), *GetNameSafe(Blueprint->ParentClass), (int32)Blueprint->Status));
	TArray<UWidget*> Widgets;
	Blueprint->WidgetTree->GetAllWidgets(Widgets);
	for (const UWidget* Widget : Widgets)
	{
		Result.Add(FString::Printf(TEXT("%s (%s)%s%s"), *Widget->GetName(), *Widget->GetClass()->GetName(),
			Widget->bIsVariable ? TEXT(" [variable]") : TEXT(""), Widget == Blueprint->WidgetTree->RootWidget ? TEXT(" [racine]") : TEXT("")));
	}
	return Result;
}
