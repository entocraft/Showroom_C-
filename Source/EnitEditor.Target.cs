using UnrealBuildTool;

public class EnitEditorTarget : TargetRules
{
	public EnitEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Enit");
		ExtraModuleNames.Add("EnitEditor");
	}
}
