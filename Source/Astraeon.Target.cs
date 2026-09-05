using UnrealBuildTool;
using System.Collections.Generic;

public class AstraeonTarget : TargetRules
{
	public AstraeonTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Astraeon");
	}
}
