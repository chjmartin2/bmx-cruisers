using UnrealBuildTool;
public class BMXCruiser : ModuleRules
{
    public BMXCruiser(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Json", "JsonUtilities", "ProceduralMeshComponent", "SlateCore", "RenderCore" });
        if (Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new string[] { "AssetRegistry", "MeshDescription", "StaticMeshDescription" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "ApplicationCore" });
    }
}

