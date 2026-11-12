using UnrealBuildTool;
public class CityCombat : ModuleRules
{
    public CityCombat(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new [] { "Core", "CoreUObject", "Engine", "InputCore", "AIModule", "NavigationSystem", "NetCore" });
        if (Target.bBuildEditor)
            PrivateDependencyModuleNames.AddRange(new [] { "UnrealEd", "MeshDescription", "StaticMeshDescription", "MeshConversion", "AssetRegistry" });
    }
}
