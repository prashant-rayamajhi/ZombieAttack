#include "PlayerAnimationSetup.h"

#if WITH_EDITOR
#include "Animation/AnimBlueprint.h"
#include "ZombieAttack/Animation/Player/PlayerAnimInstance.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_VariableGet.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "EdGraph/EdGraph.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#endif

//対象ノードを限定し、武器切替・リロード・攻撃のグラフへ変更を広げない。
int32 UPlayerAnimationSetupCommandlet::Main(const FString& _params)
{
#if WITH_EDITOR
    UAnimBlueprint* blueprint = LoadObject<UAnimBlueprint>(nullptr, TEXT("/Game/Blueprints/Animations/BPA_Player.BPA_Player"));
    if (!blueprint) { return 1; }
    const FString filename = FPackageName::LongPackageNameToFilename(blueprint->GetOutermost()->GetName(), TEXT(".uasset"));
    const FString backup = FPaths::ProjectSavedDir() / TEXT("MovementBackup/BPA_Player.uasset");
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(backup), true);
    if (!IFileManager::Get().FileExists(*backup)) { IFileManager::Get().Copy(*backup, *filename); }
    blueprint->ParentClass = UPlayerAnimInstance::StaticClass();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(blueprint);
    FKismetEditorUtilities::CompileBlueprint(blueprint);

    //同じSpeedを参照する遷移条件には触れず、三つの武器の移動ノードだけを変更する。
    TArray<UEdGraph*> graphs;
    blueprint->GetAllGraphs(graphs);
    int32 changed = 0;
    for (UEdGraph* graph : graphs)
    {
        for (UEdGraphNode* node : graph->Nodes)
        {
            //移動の循環を明示し、待機へ戻った場合も最終フレームで固まらないようにする。
            if (auto* blend = Cast<UAnimGraphNode_BlendSpacePlayer>(node)) { blend->Node.SetLoop(true); }
            if (auto* sequence = Cast<UAnimGraphNode_SequencePlayer>(node))
            {
                if (graph->GetName().Contains(TEXT("Idle"))) { sequence->Node.SetLoopAnimation(true); }
            }
            UK2Node_VariableGet* getter = Cast<UK2Node_VariableGet>(node);
            if (!getter || graph->GetName() != TEXT("Locomotion")) { continue; }
            if (getter->GetVarName() != TEXT("Speed") && getter->GetVarName() != TEXT("m_locomotionSpeed")) { continue; }
            UEdGraphPin* output = getter->FindPin(getter->GetVarName());
            if (!output) { return 2; }
            const TArray<UEdGraphPin*> links = output->LinkedTo;
            //旧Speedの識別子を残すと、再構築時に元の変数へ戻るため参照を作り直す。
            getter->VariableReference.SetFromField<FProperty>(
                FindFProperty<FProperty>(UPlayerAnimInstance::StaticClass(), TEXT("m_locomotionSpeed")), true);
            getter->ReconstructNode();
            output = getter->FindPin(TEXT("m_locomotionSpeed"));
            if (!output) { return 3; }
            for (UEdGraphPin* input : links)
            {
                input->BreakAllPinLinks();
                if (!graph->GetSchema()->TryCreateConnection(output, input)) { return 4; }
            }
            ++changed;
        }
    }
    if (changed != 1) { return 5; }
    FKismetEditorUtilities::CompileBlueprint(blueprint);
    if (blueprint->Status == BS_Error) { return 6; }
    FSavePackageArgs args;
    args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(blueprint->GetOutermost(), blueprint, *filename, args) ? 0 : 7;
#else
    return 1;
#endif
}
