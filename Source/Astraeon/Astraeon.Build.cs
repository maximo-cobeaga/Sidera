using UnrealBuildTool;

public class Astraeon : ModuleRules
{
	public Astraeon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"ProceduralMeshComponent",
			// Informes de evidencia legibles y diffables (baseline de rendimiento).
			"Json"
		});
	}
}
