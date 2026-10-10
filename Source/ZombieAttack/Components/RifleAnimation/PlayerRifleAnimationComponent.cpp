#include "PlayerRifleAnimationComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

//構えが十分に混ざった後は実際の両手を使い、脚の姿勢と合成した際の角度差も含めて求める。
FVector UPlayerRifleAnimationComponent::GetAimPoseDirection() const
{
    if (USkeletalMeshComponent* mesh = m_pTargetMesh.Get())
    {
        UAnimInstance* animation = mesh->GetAnimInstance();
        const FAnimMontageInstance* montage = animation ? animation->GetActiveInstanceForMontage(m_pActiveMontage) : nullptr;
        if (montage && montage->GetWeight() > 0.95f && (m_aimState == ERifleAimState::Raising || m_aimState == ERifleAimState::Ready))
        {
            //ワールド角ではなくメッシュ内の向きを読むため、本体の回転を重ねて補正しない。
            const FVector grip = mesh->GetBoneLocation(TEXT("RightHand"), EBoneSpaces::ComponentSpace);
            const FVector support = mesh->GetBoneLocation(TEXT("LeftHand"), EBoneSpaces::ComponentSpace);
            const FVector direction = (support - grip).GetSafeNormal2D();
            if (!direction.IsNearlyZero()) { return direction; }
        }
    }
    //構え始めの一瞬は直前の待機姿勢を使わず、構えクリップの基準方向へ向ける。
    const UAnimSequence* clip = Cast<UAnimSequence>(m_pRifleAimingIdleAnimation);
    if (!clip || !clip->GetSkeleton()) { return FVector::RightVector; }
    const FReferenceSkeleton& skeleton = clip->GetSkeleton()->GetReferenceSkeleton();
    FVector hands[2];
    const FName names[] = {TEXT("RightHand"), TEXT("LeftHand")};
    for (int32 hand = 0; hand < 2; ++hand)
    {
        int32 bone = skeleton.FindBoneIndex(names[hand]);
        if (bone == INDEX_NONE) { return FVector::RightVector; }
        FTransform pose = FTransform::Identity;
        while (bone != INDEX_NONE)
        {
            FTransform local;
            clip->GetBoneTransform(local, FSkeletonPoseBoneIndex(bone), FAnimExtractContext(0.25), false);
            pose *= local;
            bone = skeleton.GetParentIndex(bone);
        }
        hands[hand] = pose.GetLocation();
    }
    const FVector direction = (hands[1] - hands[0]).GetSafeNormal2D();
    return direction.IsNearlyZero() ? FVector::RightVector : direction;
}

//プレイヤーライフルアニメーションコンポーネントを処理します。
UPlayerRifleAnimationComponent::UPlayerRifleAnimationComponent()
    : m_pRifleIdleAnimation(nullptr), m_pRifleDownToAimAnimation(nullptr), m_pRifleAimingIdleAnimation(nullptr), m_bUseUnifiedAimingPose(true),
      m_raisePlayRate(1.15f),
      //実行状態モンタージュを処理します。
      m_pActiveMontage(nullptr), m_aimState(ERifleAimState::Inactive)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;

    //ライフル待機を処理します。
    static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> rifleIdle(TEXT("/Game/Assets/Player/Animation/AnimSequence/Rifle_Idle.Rifle_Idle"));
    static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> rifleDownToAim(
        TEXT("/Game/Assets/Player/Animation/AnimSequence/Rifle_Down_To_Aim.Rifle_Down_To_Aim"));
    static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> rifleAimingIdle(
        TEXT("/Game/Assets/Player/Animation/AnimSequence/Rifle_Aiming_Idle.Rifle_Aiming_Idle"));

    m_pRifleIdleAnimation = rifleIdle.Object;
    m_pRifleDownToAimAnimation = rifleDownToAim.Object;
    m_pRifleAimingIdleAnimation = rifleAimingIdle.Object;
}

//ARを待機姿勢から照準姿勢へ遷移させます。
void UPlayerRifleAnimationComponent::BeginAimSequence(USkeletalMeshComponent* _mesh)
{
    if (!_mesh || m_aimState == ERifleAimState::Raising || m_aimState == ERifleAimState::Ready) { return; }

    StopAimSequence(0.04f);
    m_pTargetMesh = _mesh;
    m_aimState = ERifleAimState::Raising;

    //脚のLocomotionを残したまま、上半身だけをAR姿勢へ合成します。
    SetUpperBodyBlendWeight(1.0f);
    SetComponentTickEnabled(true);
    PlayRaiseTransition();
}

//通知を受けて構え動作を完了し、発砲可能な状態へ進めます。
void UPlayerRifleAnimationComponent::CompleteAimFromNotify() { EnterAimReady(); }

//RaiseTransitionを再生します。
void UPlayerRifleAnimationComponent::PlayRaiseTransition()
{
    if (m_aimState != ERifleAimState::Raising) { return; }
    UAnimSequenceBase* aimingAnimation = m_bUseUnifiedAimingPose ? m_pRifleAimingIdleAnimation : m_pRifleDownToAimAnimation;
    if (!aimingAnimation)
    {
        StopAimSequence(0.0f);
        return;
    }

    UAnimMontage* aimingMontage =
        PlaySequence(aimingAnimation, 0.08f, 0.12f, m_bUseUnifiedAimingPose ? 1.0f : m_raisePlayRate, m_bUseUnifiedAimingPose ? 100000 : 1);
    if (!aimingMontage)
    {
        StopAimSequence(0.0f);
        return;
    }

    //統一姿勢でも入力直後は発砲を待ち、ポーズ反映後にReadyへ進めます。
    if (UWorld* world = GetWorld())
    {
        const float transitionDuration = m_bUseUnifiedAimingPose ? 0.22f : aimingAnimation->GetPlayLength() / FMath::Max(0.1f, m_raisePlayRate);
        world->GetTimerManager().SetTimer(m_sequenceTimer, this, &UPlayerRifleAnimationComponent::EnterAimReady,
                                          FMath::Max(0.05f, transitionDuration - 0.04f), false);
    }
}

//照準移行の完了通知を受け、ARの発射を許可します。
void UPlayerRifleAnimationComponent::EnterAimReady()
{
    if (m_aimState != ERifleAimState::Raising) { return; }
    //別の動作に割り込まれた構えを、残ったタイマーだけで発射可能に戻さない。
    const UAnimInstance* instance = m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptr;
    if (!instance || !m_pActiveMontage || !instance->Montage_IsPlaying(m_pActiveMontage))
    {
        StopAimSequence(0.0f);
        return;
    }
    if (UWorld* world = GetWorld())
    {
        world->GetTimerManager().ClearTimer(m_sequenceTimer);
    }

    //従来方式を選んだ場合だけ、遷移後にAiming IdleへMontageを差し替えます。
    if (!m_bUseUnifiedAimingPose)
    {
        UAnimMontage* aimingIdleMontage = PlaySequence(m_pRifleAimingIdleAnimation, 0.10f, 0.12f, 1.0f, 100000);
        if (!aimingIdleMontage)
        {
            StopAimSequence(0.0f);
            return;
        }
    }
    if (!m_pActiveMontage)
    {
        StopAimSequence(0.0f);
        return;
    }

    m_aimState = ERifleAimState::Ready;
    m_onAimReady.Broadcast();
}

//ARの発射を止め、上半身を通常の待機姿勢へ戻します。
void UPlayerRifleAnimationComponent::StopAimSequence(float _blendOutTime)
{
    if (UWorld* world = GetWorld())
    {
        world->GetTimerManager().ClearTimer(m_sequenceTimer);
    }
    UAnimInstance* animInstance = m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptr;
    if (animInstance && m_pActiveMontage)
    {
        animInstance->Montage_Stop(FMath::Max(0.0f, _blendOutTime), m_pActiveMontage);
    }

    m_pActiveMontage = nullptr;
    SetUpperBodyBlendWeight(0.0f);
    m_pTargetMesh.Reset();
    m_aimState = ERifleAimState::Inactive;
    SetComponentTickEnabled(false);
}

//リロード中を設定します。
void UPlayerRifleAnimationComponent::SetReloading(USkeletalMeshComponent* _mesh)
{
    StopAimSequence(0.06f);
    m_pTargetMesh = _mesh;
    m_aimState = ERifleAimState::Reloading;
    SetUpperBodyBlendWeight(1.0f);
    SetComponentTickEnabled(true);
}

//Aim準備完了PoseActiveかを判定します。
bool UPlayerRifleAnimationComponent::IsAimReadyPoseActive() const
{
    if (m_aimState != ERifleAimState::Ready || !m_pActiveMontage) { return false; }
    const UAnimInstance* animInstance = m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptr;
    return animInstance && animInstance->Montage_IsPlaying(m_pActiveMontage);
}

//所有者の状態変化をComponentから毎フレーム更新します。
void UPlayerRifleAnimationComponent::TickComponent(float _deltaTime, ELevelTick _tickType, FActorComponentTickFunction* _tickFunction)
{
    Super::TickComponent(_deltaTime, _tickType, _tickFunction);

    //構え中にモンタージュが止まったら待機へ戻し、次の入力で構え直せるようにする。
    if (m_aimState == ERifleAimState::Raising || m_aimState == ERifleAimState::Ready)
    {
        const UAnimInstance* instance = m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptr;
        if (!instance || !m_pActiveMontage || !instance->Montage_IsPlaying(m_pActiveMontage))
        {
            StopAimSequence(0.0f);
            return;
        }
    }
    //AnimBPが値を更新しても、照準・リロード中は上半身ウェイトを維持します。
    if (m_aimState != ERifleAimState::Inactive)
    {
        SetUpperBodyBlendWeight(1.0f);
    }
}

//ゲーム終了時に登録済みの通知とタイマーを解除します。
void UPlayerRifleAnimationComponent::EndPlay(const EEndPlayReason::Type _endPlayReason)
{
    StopAimSequence(0.0f);
    m_onAimReady.Clear();
    Super::EndPlay(_endPlayReason);
}

//Sequenceを再生します。
UAnimMontage* UPlayerRifleAnimationComponent::PlaySequence(UAnimSequenceBase* _animation, float _blendInTime, float _blendOutTime, float _playRate,
                                                           int32 _loopCount)
{
    UAnimInstance* animInstance = m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptr;
    if (!animInstance || !_animation) { return nullptr; }
    if (m_pActiveMontage)
    {
        animInstance->Montage_Stop(0.04f, m_pActiveMontage);
    }

    m_pActiveMontage = animInstance->PlaySlotAnimationAsDynamicMontage(_animation, TEXT("UpperBody"), _blendInTime, _blendOutTime,
                                                                       FMath::Max(0.1f, _playRate), FMath::Max(1, _loopCount), 0.0f);
    return m_pActiveMontage;
}

//UpperBodyBlendWeightをゲーム内の対象へ反映します。
void UPlayerRifleAnimationComponent::SetUpperBodyBlendWeight(float _blendWeight) const
{
    UAnimInstance* animInstance = m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptr;
    if (!animInstance) { return; }

    //BPA_Playerに公開された合成ウェイトがある場合だけ安全に更新します。
    FFloatProperty* blendWeightProperty = FindFProperty<FFloatProperty>(animInstance->GetClass(), TEXT("UpperBodyAlpha"));
    if (blendWeightProperty)
    {
        blendWeightProperty->SetPropertyValue_InContainer(animInstance, FMath::Clamp(_blendWeight, 0.0f, 1.0f));
    }
}
