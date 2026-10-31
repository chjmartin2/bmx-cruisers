using UnrealBuildTool;
using System.Collections.Generic;
public class BMXCruiserEditorTarget : TargetRules
{
    public BMXCruiserEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("BMXCruiser");
    }
}
