using UnrealBuildTool;
public class SoulCityGameEditorTarget : TargetRules
{
    public SoulCityGameEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("CityCombat");
    }
}
