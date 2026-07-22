using UnrealBuildTool;
using System.Collections.Generic;

public class GovernedUnrealProjectEditorTarget : TargetRules
{
	public GovernedUnrealProjectEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("GovernedUnrealProject");
	}
}
