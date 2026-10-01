#include "BossChara.h"
#include "ZombieAttack/Player/PlayerChara.h"
#include "ZombieAttack/AIController/BossAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "ZombieAttack/Animation/Enemy/EnemyAnimationTiming.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "Engine/World.h"

//パンチと突進は手足の接触、スラムは接地時の範囲判定を使う。
void ABossChara::SetAttackCollisionEnabled(bool _bEnabled)
{
    const bool contactMove = m_currentPattern == EBossAttackPattern::ChargeRush || m_currentPattern == EBossAttackPattern::BackStep;
    const bool allowBox = (contactMove || m_currentPattern == EBossAttackPattern::LightCombo) && !m_bRecovering && !m_bIsTransitioning;
    Super::SetAttackCollisionEnabled(_bEnabled && allowBox);
}

//攻撃パターンを選択する関数
EBossAttackPattern ABossChara::ChoosePattern() const
{
    //ランダムな値を生成して攻撃パターンを選択
    const float randomValue = FMath::FRand();

    //フェーズ2の場合、攻撃パターンの確率を変更
    if (m_bPhaseTwo)
    {
        if (randomValue < 0.45f) { return EBossAttackPattern::LightCombo; }
        if (randomValue < 0.70f) { return EBossAttackPattern::PowerSlam; }
        if (randomValue < 0.90f) { return EBossAttackPattern::ChargeRush; }
        return EBossAttackPattern::BackStep;
    }

    //フェーズ1の場合、攻撃パターンの確率を設定
    if (randomValue < 0.45f) { return EBossAttackPattern::LightCombo; }
    if (randomValue < 0.65f) { return EBossAttackPattern::PowerSlam; }
    if (randomValue < 0.85f) { return EBossAttackPattern::ChargeRush; }
    return EBossAttackPattern::BackStep;
}

//指定された攻撃パターンを実行する関数
void ABossChara::ExecutePattern(EBossAttackPattern _pattern)
{
    //攻撃中フラグを設定
    m_bAttacking = true;

    //攻撃パターンに応じて処理を分岐
    switch (_pattern)
    {
    case EBossAttackPattern::LightCombo: DoLightCombo(); break;

    case EBossAttackPattern::PowerSlam: DoPowerSlam(); break;

    case EBossAttackPattern::ChargeRush: DoChargeRush(); break;

    case EBossAttackPattern::BackStep: DoBackStep(); break;

    default: FinishBossAttack(); break;
    }
}

//攻撃パターンの処理を実行する関数
void ABossChara::DoLightCombo() { StartComboAttack(); }

//コンボ攻撃を開始する関数
void ABossChara::StartComboAttack()
{
    m_currentComboIndex = 0;
    m_bComboHitConfirmed = false;
    PlayComboStep();
}

//コンボ攻撃のステップを再生する関数
void ABossChara::PlayComboStep()
{
    m_bLightComboAreaResolvedThisStep = false;
    UAnimMontage* montageToPlay = nullptr;

    //コンボ攻撃の現在のステップに対応するアニメーションモンタージュを取得
    if (m_comboAttackMontages.IsValidIndex(m_currentComboIndex))
    {
        montageToPlay = m_comboAttackMontages[m_currentComboIndex];
    }
    if (!IsMontageCompatible(montageToPlay))
    {
        montageToPlay = nullptr;
    }

    //コンボ攻撃のステップが無効な場合、ライトコンボのデフォルトモンタージュを使用
    if (!montageToPlay && IsMontageCompatible(m_pLightComboMontage))
    {
        montageToPlay = m_pLightComboMontage;
    }

    //アニメーションモンタージュを再生するか、攻撃を終了する
    if (montageToPlay)
    {
        const float montageDuration = PlayAnimMontage(montageToPlay);
        if (montageDuration <= 0.0f)
        {
            FinishBossAttack();
            return;
        }

        GetWorldTimerManager().ClearTimer(m_lightComboEffectTimer);
        if (!EnemyAnimationTiming::HasHitNotify(montageToPlay))
        {
            GetWorldTimerManager().SetTimer(m_lightComboEffectTimer, this, &ABossChara::ExecuteLightComboAreaAttack,
                                            FMath::Max(0.05f, montageDuration * 0.55f), false);
        }
        TWeakObjectPtr<ABossChara> weakThis = this;
        GetWorldTimerManager().SetTimer(m_attackAnimationSafetyTimer,
                                        FTimerDelegate::CreateLambda(
                                            [weakThis]()
                                            {
                                                if (weakThis.IsValid() && weakThis->m_bAttacking)
                                                {
                                                    weakThis->FinishBossAttack();
                                                }
                                            }),
                                        montageDuration + 0.08f, false);
    }
    else
    {
        FinishBossAttack();
    }
}

//コンボ攻撃の分岐を要求する関数
void ABossChara::RequestComboBranchFromNotify()
{
    if (!m_bAttacking || m_bRecovering || m_bIsTransitioning || IsDead()) { return; }
    //攻撃判定を無効化
    SetAttackCollisionEnabled(false);

    //連撃用の通知でスラムや突進を途中終了させず、各攻撃の終了時刻まで再生する
    if (m_currentPattern != EBossAttackPattern::LightCombo) { return; }

    //コンボ攻撃がヒットしていない場合、攻撃を終了
    if (!m_bComboHitConfirmed || !CanHitPlayer(GetContactAttackRange()))
    {
        FinishBossAttack();
        return;
    }

    //次のコンボステップのインデックスを計算
    const int32 nextComboIndex = m_currentComboIndex + 1;
    if (!m_comboAttackMontages.IsValidIndex(nextComboIndex))
    {
        FinishBossAttack();
        return;
    }

    //次のコンボステップを再生
    m_currentComboIndex = nextComboIndex;
    m_bComboHitConfirmed = false;
    PlayComboStep();
}

//パワースラム攻撃を実行する関数
void ABossChara::DoPowerSlam()
{
    //AM_MutantSlamを再生するたび、今回の衝撃エフェクトを未再生状態に戻します。
    m_bPowerSlamEffectSpawned = false;
    m_bSlamHitResolved = false;

    //パワースラムのアニメーションモンタージュを再生するか、攻撃を終了する
    if (IsMontageCompatible(m_pPowerSlamMontage))
    {
        const float montageDuration = PlayAnimMontage(m_pPowerSlamMontage);
        if (montageDuration <= 0.0f)
        {
            FinishBossAttack();
            return;
        }

        //ダメージNotifyとは分離し、モンタージュ終盤で地面の衝撃波だけを生成します。
        GetWorldTimerManager().ClearTimer(m_powerSlamEffectTimer);
        if (!EnemyAnimationTiming::HasHitNotify(m_pPowerSlamMontage))
        {
            GetWorldTimerManager().SetTimer(m_powerSlamEffectTimer, this, &ABossChara::PerformBossAttackHit,
                                            FMath::Max(0.01f, montageDuration * 0.55f), false);
        }
        TWeakObjectPtr<ABossChara> weakThis = this;
        GetWorldTimerManager().SetTimer(m_attackAnimationSafetyTimer,
                                        FTimerDelegate::CreateLambda(
                                            [weakThis]()
                                            {
                                                if (weakThis.IsValid())
                                                {
                                                    weakThis->FinishBossAttack();
                                                }
                                            }),
                                        montageDuration + 0.08f, false);
    }
    else
    {
        FinishBossAttack();
    }
}

//チャージラッシュ攻撃を実行する関数
void ABossChara::DoChargeRush()
{
    //プレイヤーキャラクターを取得
    APlayerChara* player = GetCombatTarget();
    if (!player)
    {
        FinishBossAttack();
        return;
    }
    if (!IsMontageCompatible(m_pChargeRushMontage))
    {
        FinishBossAttack();
        return;
    }
    //別モデル向けの短い走行を全身へ混ぜず、このボスの走行を二周期使う。
    UAnimInstance* instance = GetMesh()->GetAnimInstance();
    UAnimSequence* run = GetRunAnimation();
    UAnimMontage* rush = instance && run ?
        instance->PlaySlotAnimationAsDynamicMontage(run, TEXT("DefaultSlot"), 0.08f, 0.16f, 1.0f, 2) : nullptr;
    if (!rush)
    {
        FinishBossAttack();
        return;
    }
    if (m_pChargeVFX)
    {
        //エフェクト位置を保持します。
        FVector effectLocation;
        //エフェクト回転を保持します。
        FRotator effectRotation;
        ResolveGroundEffectTransform(0.0f, effectLocation, effectRotation);
        //SystemAt位置を作成します。
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_pChargeVFX, effectLocation, effectRotation, FVector(0.22f));
    }

    //対象位置を保持します。
    const FVector targetLocation = m_requestedTargetLocation.IsNearlyZero() ? player->GetActorLocation() : m_requestedTargetLocation;
    //方向を保持します。
    m_rushDirection = (targetLocation - GetActorLocation()).GetSafeNormal2D();
    m_rushElapsed = 0.0f;
    m_rushDuration = run->GetPlayLength() * 2.0f;
    m_bRushContactStarted = false;
    SetActorRotation(FRotator(0.0f, m_rushDirection.Rotation().Yaw, 0.0f));
    TWeakObjectPtr<ABossChara> weakThis = this;
    GetWorldTimerManager().SetTimer(m_chargeTimer,
                                    FTimerDelegate::CreateLambda(
                                        [weakThis]()
                                        {
                                            if (weakThis.IsValid())
                                            {
                                                weakThis->FinishBossAttack();
                                            }
                                        }),
                                    FMath::Max(0.2f, m_rushDuration), false);
}

//吹き飛ばしで滑らせず、前方の障害物に当たれば止まる通常の移動で突進する。
void ABossChara::UpdateChargeRush(float _deltaTime)
{
    if (!m_bAttacking || m_bRecovering || m_bIsTransitioning || IsDead() || m_currentPattern != EBossAttackPattern::ChargeRush) { return; }
    m_rushElapsed += _deltaTime;
    if (m_rushElapsed < 0.18f || m_rushDuration <= 0.36f) { return; }
    if (!m_bRushContactStarted)
    {
        m_bRushContactStarted = true;
        ResetAttackHitForNewSwing();
        PrepareAttackContact(TEXT("RightHand"));
        SetAttackCollisionEnabled(true);
    }
    UCharacterMovementComponent* movement = GetCharacterMovement();
    movement->MaxWalkSpeed = FMath::Min(600.0f, m_chargeDistance / (m_rushDuration - 0.18f));
    AddMovementInput(m_rushDirection, 1.0f);
    //壁や大きな障害物が直前にあれば、足だけ走り続けず突進を打ち切る。
    FHitResult obstacle;
    FCollisionQueryParams query(SCENE_QUERY_STAT(BossRush), false, this);
    if (APlayerChara* player = GetCombatTarget()) { query.AddIgnoredActor(player); }
    const float radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float height = GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.8f;
    if (GetWorld()->SweepSingleByChannel(obstacle, GetActorLocation(), GetActorLocation() + m_rushDirection * 70.0f,
        FQuat::Identity, ECC_Visibility, FCollisionShape::MakeCapsule(radius, FMath::Max(radius, height)), query))
    {
        GetMesh()->GetAnimInstance()->Montage_Stop(0.15f);
        FinishBossAttack();
    }
}

//バックステップ攻撃を実行する関数
void ABossChara::DoBackStep()
{
    if (!IsMontageCompatible(m_pBackStepMontage))
    {
        FinishBossAttack();
        return;
    }
    const float montageDuration = PlayAnimMontage(m_pBackStepMontage);
    if (montageDuration <= 0.0f)
    {
        FinishBossAttack();
        return;
    }
    if (m_pBackStepVFX)
    {
        //エフェクト位置を保持します。
        FVector effectLocation;
        //エフェクト回転を保持します。
        FRotator effectRotation;
        ResolveGroundEffectTransform(0.0f, effectLocation, effectRotation);
        //SystemAt位置を作成します。
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_pBackStepVFX, effectLocation, effectRotation, FVector(0.18f));
    }

    //バックステップの距離を計算して後方にインパルスを加える
    const FVector backward = -GetActorForwardVector() * FMath::Min(350.0f, m_chargeDistance * 0.5f) / FMath::Max(0.2f, montageDuration);
    if (UCharacterMovementComponent* movementComponent = GetCharacterMovement())
    {
        LaunchCharacter(backward, true, true);
    }

    //攻撃を終了する
    GetWorldTimerManager().SetTimer(m_chargeTimer, this, &ABossChara::FinishBossAttack, FMath::Max(0.2f, montageDuration), false);
}

//攻撃を終了する関数
void ABossChara::FinishBossAttack()
{
    if (IsDead() || m_bRecovering || m_bIsTransitioning || !m_bAttacking) { return; }
    //AM_MutantSlam側の終了通知が早くても、最後の地面衝撃だけは欠落させません。
    if (m_bAttacking && m_currentPattern == EBossAttackPattern::PowerSlam && !m_bPowerSlamEffectSpawned)
    {
        SpawnPowerSlamEffect();
    }

    GetWorldTimerManager().ClearTimer(m_chargeTimer);
    GetWorldTimerManager().ClearTimer(m_powerSlamEffectTimer);
    GetWorldTimerManager().ClearTimer(m_lightComboEffectTimer);
    GetWorldTimerManager().ClearTimer(m_attackAnimationSafetyTimer);
    m_bRecovering = true;
    //攻撃中フラグを解除
    SetAttackCollisionEnabled(false);
    EndAttackVFXWindow();
    //キャラクター移動を返します。
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->MaxWalkSpeed = 0.0f;
    }

    //攻撃クールダウンタイマーを設定
    const float cooldown = m_bPhaseTwo ? m_phaseTwoAttackCooldown : m_attackCooldown;
    TWeakObjectPtr<ABossChara> weakThis = this;

    //クールダウン後に攻撃状態をリセットし、BossAIControllerに通知
    GetWorldTimerManager().SetTimer(m_attackCooldownTimer,
                                    FTimerDelegate::CreateLambda(
                                        [weakThis]()
                                        {
                                            if (!weakThis.IsValid() || weakThis->IsDead()) { return; }

                                            weakThis->m_bAttacking = false;
                                            weakThis->m_bRecovering = false;
                                            weakThis->SetCombatActionActive(false);
                                            weakThis->GetCharacterMovement()->MaxWalkSpeed = weakThis->m_chaseRunSpeed;
                                            weakThis->m_currentComboIndex = 0;
                                            weakThis->m_bComboHitConfirmed = false;
                                            weakThis->m_bLightComboAreaResolvedThisStep = false;
                                            if (ABossAIController* bossAIController = Cast<ABossAIController>(weakThis->GetController()))
                                            {
                                                bossAIController->NotifyAttackFinished();
                                            }
                                        }),
                                    FMath::Max(0.01f, cooldown), false);
}

//死亡や段階移行の前に、以前の攻撃から残った命中・終了の予約をすべて取り消す
void ABossChara::ClearCombatTimers()
{
    GetWorldTimerManager().ClearTimer(m_chargeTimer);
    GetWorldTimerManager().ClearTimer(m_powerSlamEffectTimer);
    GetWorldTimerManager().ClearTimer(m_lightComboEffectTimer);
    GetWorldTimerManager().ClearTimer(m_attackAnimationSafetyTimer);
    GetWorldTimerManager().ClearTimer(m_attackCooldownTimer);
    GetWorldTimerManager().ClearTimer(m_phaseTransitionTimer);
}
