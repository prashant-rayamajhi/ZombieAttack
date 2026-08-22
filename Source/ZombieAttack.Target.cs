using UnrealBuildTool;

//ゲーム実行用のビルド設定をまとめます。
public class ZombieAttackTarget : TargetRules
{
    //ゲーム本体で使うモジュールとビルド方式を設定します。
    public ZombieAttackTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("ZombieAttack");
    }
}
