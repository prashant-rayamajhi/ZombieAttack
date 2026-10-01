//著作権表記: Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

//ゲーム本体が利用するモジュールをまとめます。
public class ZombieAttack : ModuleRules
{
    //実行時に必要な各モジュールを登録します。
    public ZombieAttack(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        //ゲーム本体から直接参照するモジュールを登録します。
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "AIModule",
                                                            "GameplayTasks", "NavigationSystem", "Niagara", "Slate", "SlateCore",
                                                            "DeveloperSettings", "AnimGraphRuntime" });
        //画面の描画テストだけで必要なモジュールをEditorへ限定する。
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] { "RenderCore", "RHI", "ImageCore", "UnrealEd", "BlueprintGraph", "AnimGraph" });
        }
    }
}
