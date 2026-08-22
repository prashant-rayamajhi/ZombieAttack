using UnrealBuildTool;

//Unreal Editorで使うビルド設定をまとめます。
public class ZombieAttackEditorTarget : TargetRules
{
    //ZombieAttackEditorTargetは、ZombieAttackEditorTargetの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    public ZombieAttackEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("ZombieAttack");
    }
}
