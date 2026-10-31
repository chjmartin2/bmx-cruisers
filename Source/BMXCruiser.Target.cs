using UnrealBuildTool;
using System.Collections.Generic;
public class BMXCruiserTarget : TargetRules
{
    public BMXCruiserTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("BMXCruiser");
    }
}
