using UnrealBuildTool;

public class Enit : ModuleRules
{
	public Enit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Permet d'ecrire #include "Player/ShowroomPawn.h" depuis n'importe quel fichier.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"HeadMountedDisplay",
			"XRBase",
			"UMG",
			"Slate",
			"SlateCore",
			"NavigationSystem",
			"Niagara",
			"DeveloperSettings",
			"NetCore",
			"ProceduralMeshComponent"
		});
	}
}
