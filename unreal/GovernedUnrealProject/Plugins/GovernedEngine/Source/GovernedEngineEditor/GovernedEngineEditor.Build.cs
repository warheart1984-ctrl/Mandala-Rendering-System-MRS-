using UnrealBuildTool;

public class GovernedEngineEditor : ModuleRules
{
	public GovernedEngineEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"LevelSequence",
			"MovieScene",
			"MovieRenderPipelineCore",
			"MovieRenderPipelineRenderPasses",
			"MovieRenderPipelineEditor",
			"GovernedEngine",
			"Json",
			"JsonUtilities",
			"ProceduralMeshComponent",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
		});
	}
}
