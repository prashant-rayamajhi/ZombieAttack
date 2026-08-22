#include "PlayerRifleAnimationComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

//プレイヤーライフルアニメーションコンポーネントを処理します。
UPlayerRifleAnimationComponent::UPlayerRifleAnimationComponent()
    : m_pRifleIdleAnimation(nullptr), m_pRifleDownToAimAnimation(nullptr), m_pRifleAimingIdleAnimation(nullptr), b_mUseUnifiedAimingPose(true),
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
    //「!_mesh || m_aimState == ERifleAimState::Raising || m_aimState == ERifleAimState::Ready」が成立するとき、StopAimSequenceを呼び出します。
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
    //「m_aimState != ERifleAimState::Raising」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (m_aimState != ERifleAimState::Raising) { return; }

    //aimingAnimationは、b_mUseUnifiedAimingPose ? m_pRifleAimingIdleAnimation : m_pRifleDownToA…から取得した参照を後続の呼び出しで使います。
    UAnimSequenceBase* aimingAnimation = b_mUseUnifiedAimingPose ? m_pRifleAimingIdleAnimation : m_pRifleDownToAimAnimation;
    if (!aimingAnimation)
    {
        StopAimSequence(0.0f);
        return;
    }

    UAnimMontage* aimingMontage =
        PlaySequence(aimingAnimation, 0.08f, 0.12f, b_mUseUnifiedAimingPose ? 1.0f : m_raisePlayRate, b_mUseUnifiedAimingPose ? 100000 : 1);
    if (!aimingMontage)
    {
        StopAimSequence(0.0f);
        return;
    }

    //統一姿勢でも入力直後は発砲を待ち、ポーズ反映後にReadyへ進めます。
    if (UWorld* world = GetWorld())
    {
        //transitionDurationは、b_mUseUnifiedAimingPose ? 0.22f : aimingAnimation->GetPlayLength() / FM…から算出した数値を後続の判定または計算に使います。
        const float transitionDuration = b_mUseUnifiedAimingPose ? 0.22f : aimingAnimation->GetPlayLength() / FMath::Max(0.1f, m_raisePlayRate);
        world->GetTimerManager().SetTimer(m_sequenceTimer, this, &UPlayerRifleAnimationComponent::EnterAimReady,
                                          FMath::Max(0.05f, transitionDuration - 0.04f), false);
    }
}

//照準移行の完了通知を受け、ARの発射を許可します。
void UPlayerRifleAnimationComponent::EnterAimReady()
{
    //「m_aimState != ERifleAimState::Raising」が成立するとき、続けて「UWorld* world = GetWorld()」を判定します。
    if (m_aimState != ERifleAimState::Raising) { return; }

    //「UWorld* world = GetWorld()」が成立するとき、GetTimerManagerを呼び出します。
    if (UWorld* world = GetWorld())
    {
        world->GetTimerManager().ClearTimer(m_sequenceTimer);
    }

    //従来方式を選んだ場合だけ、遷移後にAiming IdleへMontageを差し替えます。
    if (!b_mUseUnifiedAimingPose)
    {
        //aimingIdleMontageは、PlaySequence(m_pRifleAimingIdleAnimation, 0.10f, 0.12f, 1.0f, 100000)から取得した参照を後続の呼び出しで使います。
        UAnimMontage* aimingIdleMontage = PlaySequence(m_pRifleAimingIdleAnimation, 0.10f, 0.12f, 1.0f, 100000);
        if (!aimingIdleMontage)
        {
            StopAimSequence(0.0f);
            return;
        }
    }

    //「!m_pActiveMontage」が成立するとき、StopAimSequenceを呼び出します。
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
    //「UWorld* world = GetWorld()」が成立するとき、GetTimerManagerを呼び出します。
    if (UWorld* world = GetWorld())
    {
        world->GetTimerManager().ClearTimer(m_sequenceTimer);
    }

    //animInstanceは、m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptrから取得した参照を後続の呼び出しで使います。
    UAnimInstance* animInstance = m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptr;
    //「animInstance && m_pActiveMontage」が成立するとき、Montage_Stopを呼び出します。
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
    //「m_aimState != ERifleAimState::Ready || !m_pActiveMontage」が成立するとき、IsValidを呼び出します。
    if (m_aimState != ERifleAimState::Ready || !m_pActiveMontage) { return false; }

    //animInstanceは、m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptrから取得した参照を後続の呼び出しで使います。
    const UAnimInstance* animInstance = m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptr;
    return animInstance && animInstance->Montage_IsPlaying(m_pActiveMontage);
}

//所有者の状態変化をComponentから毎フレーム更新します。
void UPlayerRifleAnimationComponent::TickComponent(float _deltaTime, ELevelTick _tickType, FActorComponentTickFunction* _tickFunction)
{
    Super::TickComponent(_deltaTime, _tickType, _tickFunction);

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
    //animInstanceは、m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptrから取得した参照を後続の呼び出しで使います。
    UAnimInstance* animInstance = m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptr;
    //「!animInstance || !_animation」が成立するとき、続けて「m_pActiveMontage」を判定します。
    if (!animInstance || !_animation) { return nullptr; }

    //「m_pActiveMontage」が成立するとき、Montage_Stopを呼び出します。
    if (m_pActiveMontage)
    {
        animInstance->Montage_Stop(0.04f, m_pActiveMontage);
    }

    m_pActiveMontage = animInstance->PlaySlotAnimationAsDynamicMontage(_animation, TEXT("UpperBody"), _blendInTime, _blendOutTime,
                                                                       FMath::Max(0.1f, _playRate), FMath::Max(1, _loopCount), 0.0f);
    //m_pActiveMontageは、直後の初期化結果を、同じスコープ内でこの名前を参照する計算や関数呼び出しへ渡すために使います。
    return m_pActiveMontage;
}

//UpperBodyBlendWeightをゲーム内の対象へ反映します。
void UPlayerRifleAnimationComponent::SetUpperBodyBlendWeight(float _blendWeight) const
{
    //animInstanceは、m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptrから取得した参照を後続の呼び出しで使います。
    UAnimInstance* animInstance = m_pTargetMesh.IsValid() ? m_pTargetMesh->GetAnimInstance() : nullptr;
    //「!animInstance」が成立するとき、GetClassを呼び出します。
    if (!animInstance) { return; }

    //BPA_Playerに公開された合成ウェイトがある場合だけ安全に更新します。
    FFloatProperty* blendWeightProperty = FindFProperty<FFloatProperty>(animInstance->GetClass(), TEXT("UpperBodyAlpha"));
    //「blendWeightProperty」が成立するとき、SetPropertyValue_InContainerを呼び出します。
    if (blendWeightProperty)
    {
        blendWeightProperty->SetPropertyValue_InContainer(animInstance, FMath::Clamp(_blendWeight, 0.0f, 1.0f));
    }
}
