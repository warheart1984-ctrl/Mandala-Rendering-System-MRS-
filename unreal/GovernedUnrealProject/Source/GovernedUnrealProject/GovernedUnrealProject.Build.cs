using UnrealBuildTool;

public class GovernedUnrealProject : ModuleRules
{
	public GovernedUnrealProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GovernedEngine",
		});
	}
}
