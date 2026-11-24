using UnrealBuildTool;

// Module EDITEUR uniquement (jamais dans le jeu final) : outils pour fabriquer les Widget Blueprints.
public class EnitEditor : ModuleRules
{
	public EnitEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Enit"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"UMG",
			"UMGEditor",
			"AssetTools",
			"Kismet",
			"Slate",
			"SlateCore",
			"RenderCore"
		});
	}
}
