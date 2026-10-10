#include "EnemyChara.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ZombieAttack/Animation/Enemy/EnemyAnimInstance.h"

//コンストラクタで参照を設定し、パッケージにも各敵の移動クリップを含める
void AEnemyChara::SetLocomotionAssets(const TCHAR* _folder, const TCHAR* _idle, const TCHAR* _walk, const TCHAR* _run, const TCHAR* _alert)
{
    const auto loadSequence = [_folder](const TCHAR* _name)
    {
        return LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("%s/%s.%s"), _folder, _name, _name));
    };
    m_idleAnimation = loadSequence(_idle);
    m_walkAnimation = loadSequence(_walk);
    m_runAnimation = loadSequence(_run);
    m_alertAnimation = loadSequence(_alert);
    //種別フォルダ内の専用クリップを参照し、別モデルのアニメーションを直接混ぜない。
    const FString folder = FPaths::GetPath(FString(_folder)) / TEXT("Generated");
    m_strafeLeft = LoadObject<UAnimSequence>(nullptr, *(folder / TEXT("Strafe_Left.Strafe_Left")));
    m_strafeRight = LoadObject<UAnimSequence>(nullptr, *(folder / TEXT("Strafe_Right.Strafe_Right")));
}

//同一Skeletonのクリップが揃った時だけ共通の移動評価へ切り替える
void AEnemyChara::InitializeEnemyAnimation()
{
    USkeletalMeshComponent* mesh = GetMesh();
    const USkeletalMesh* asset = mesh ? mesh->GetSkeletalMeshAsset() : nullptr;
    if (mesh)
    {
        //背後の敵も手足の接触位置を更新し、最後に画面へ映った位置で攻撃判定しない。
        mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        //短い接触区間を間引かず、骨の更新後に行う攻撃掃引と同じフレームへ揃える。
        mesh->bEnableUpdateRateOptimizations = false;
    }
    if (!asset || !m_idleAnimation || !m_walkAnimation || !m_runAnimation) { return; }
    for (const UAnimSequence* sequence : {m_idleAnimation.Get(), m_walkAnimation.Get(), m_runAnimation.Get()})
    {
        if (sequence->GetSkeleton() != asset->GetSkeleton()) { return; }
    }
    //BPで横移動を差し替えた場合も、別の骨構成を移動姿勢へ混ぜない。
    if (m_strafeLeft && m_strafeLeft->GetSkeleton() != asset->GetSkeleton()) { m_strafeLeft = nullptr; }
    if (m_strafeRight && m_strafeRight->GetSkeleton() != asset->GetSkeleton()) { m_strafeRight = nullptr; }
    mesh->SetAnimInstanceClass(UEnemyAnimInstance::StaticClass());
    if (UAnimInstance* instance = mesh->GetAnimInstance())
    {
        //移動距離はCharacterMovementへ統一し、クリップ内の移動との二重加算を防ぐ
        instance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
    }
}

//咆哮クリップの長さを停止時間へ使い、途中で走り出すタイマーのずれをなくす
float AEnemyChara::PlayAlertAnimation()
{
    USkeletalMeshComponent* mesh = GetMesh();
    UAnimInstance* instance = mesh ? mesh->GetAnimInstance() : nullptr;
    if (m_bIsDead || m_bIsAttacking || !instance || !m_alertAnimation) { return 0.0f; }
    if (m_alertAnimation->GetSkeleton() != mesh->GetSkeletalMeshAsset()->GetSkeleton()) { return 0.0f; }
    //離れた集団の呼び声も同じ拍にならないよう、動作の速さを小さく変える。
    const float rate = FMath::FRandRange(0.92f, 1.08f);
    UAnimMontage* montage = instance->PlaySlotAnimationAsDynamicMontage(m_alertAnimation, TEXT("DefaultSlot"), 0.12f, 0.16f, rate);
    return montage ? montage->GetPlayLength() / rate : 0.0f;
}
