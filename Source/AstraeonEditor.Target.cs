using UnrealBuildTool;
using System.Collections.Generic;

public class AstraeonEditorTarget : TargetRules
{
	public AstraeonEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Astraeon");
	}
}
