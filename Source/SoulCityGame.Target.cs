using UnrealBuildTool;
public class SoulCityGameTarget : TargetRules
{
    public SoulCityGameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("CityCombat");
    }
}
