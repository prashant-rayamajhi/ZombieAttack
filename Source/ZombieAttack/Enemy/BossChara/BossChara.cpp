#include "BossChara.h"

#include "ZombieAttack/PickUp/PickUpBase.h"
#include "ZombieAttack/Player/PlayerChara.h"
#include "ZombieAttack/AIController/BossAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "ZombieAttack/Animation/Enemy/EnemyAnimationTiming.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

//コンストラクタ
ABossChara::ABossChara()
    : m_phaseTwoThreshold(0.5f), m_pLightComboMontage(nullptr), m_pPowerSlamMontage(nullptr), m_pChargeRushMontage(nullptr),
      m_pBackStepMontage(nullptr), m_pPhaseTransitionMontage(nullptr), m_pRoarSound(nullptr), m_pImpactSound(nullptr), m_pWeaponDropClass(nullptr),
      m_weaponDropValue(0.0f), m_attackCooldown(3.0f), m_phaseTwoAttackCooldown(1.0f), m_meleeRange(200.0f), m_chargeDistance(600.0f),
      m_powerSlamForwardOffset(110.0f), m_powerSlamEffectLeadTime(0.18f),
      m_pComboImpactVFX(nullptr), m_pPowerSlamVFX(nullptr), m_pChargeVFX(nullptr), m_pBackStepVFX(nullptr), m_pPhaseTransitionVFX(nullptr),
      m_currentPattern(EBossAttackPattern::LightCombo), m_bPhaseTwo(false), m_bAttacking(false), m_bIsTransitioning(false),
      m_bPhaseTransitionTriggered(false), m_bComboHitConfirmed(false), m_bLightComboAreaResolvedThisStep(false), m_bPowerSlamEffectSpawned(false),
      m_currentComboIndex(0), m_requestedTargetLocation(FVector::ZeroVector)
{
    //魔法・雷・魂のAuraを避け、物理的な衝撃と風圧だけに統一します。
    m_pComboImpactVFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave"));
    m_pPowerSlamVFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave"));
    m_pChargeVFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Air.NS_Free_Spells_Aura_Air"));
    m_pBackStepVFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Air.NS_Free_Spells_Aura_Air"));
    m_pPhaseTransitionVFX =
        LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave"));

    //キャラクターの移動速度を初期化
    if (UCharacterMovementComponent* movementComponent = GetCharacterMovement())
    {
        //BlendSpaceの歩行サンプル速度に合わせ、足の滑りを抑える
        movementComponent->MaxWalkSpeed = 125.0f;
    }
}

//ゲーム開始時の処理

void ABossChara::BeginPlay()
{
    //ゲーム開始時に必要な参照を取得し、初期状態を整えます。
    Super::BeginPlay();

    //中ボスと最終ボスは同じ基準HPに統一します。
    //BP側に古い値が保存されていても、実ゲームでは必ず500になります。
    m_maxHp = 500.0f;
    m_hp = m_maxHp;
    m_onHealthChanged.Broadcast();
}

//毎フレーム呼ばれる関数
void ABossChara::Tick(float _deltaTime)
{
    //フレームごとの経過時間を使って、移動や表示の変化を更新します。
    Super::Tick(_deltaTime);
    UpdateChargeRush(_deltaTime);
}

//ダメージを受けたときの処理
float ABossChara::TakeDamage(float _damageAmount, FDamageEvent const& _damageEvent, AController* _eventInstigator, AActor* _damageCauser)
{
    //親クラスのTakeDamageを呼び出してダメージを計算
    const float damage = Super::TakeDamage(_damageAmount, _damageEvent, _eventInstigator, _damageCauser);

    //ダメージが0以下の場合は処理を終了
    if (damage <= 0.0f) { return 0.0f; }

    //BossAIControllerにダメージ通知を送信
    if (ABossAIController* bossAIController = Cast<ABossAIController>(GetController()))
    {
        bossAIController->NotifyBossDamaged(damage);
    }
    if (IsDead()) { return damage; }

    //フェーズ2への移行条件をチェック
    if (!m_bPhaseTwo && GetHealthRatio() <= m_phaseTwoThreshold)
    {
        //前の攻撃予約を消してから咆哮へ移り、途中のパンチや突進を再開させない
        ClearCombatTimers();
        m_bRecovering = false;
        SetCombatActionActive(true);
        if (AEnemyAIController* controller = Cast<AEnemyAIController>(GetController())) { controller->NotifyAttackStarted(); }
        if (UAnimInstance* instance = GetMesh()->GetAnimInstance()) { instance->Montage_Stop(0.1f); }
        //フェーズ2への移行を開始
        m_bPhaseTwo = true;
        m_bPhaseTransitionTriggered = true;
        m_bIsTransitioning = true;
        m_bAttacking = true;
        SetAttackCollisionEnabled(false);

        //フェーズ移行中のフラグを設定
        if (m_pRoarSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, m_pRoarSound, GetActorLocation());
        }
        if (m_pPhaseTransitionVFX)
        {
            //エフェクト位置を保持します。
            FVector effectLocation;
            //エフェクト回転を保持します。
            FRotator effectRotation;
            ResolveGroundEffectTransform(0.0f, effectLocation, effectRotation);
            //SystemAt位置を作成します。
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_pPhaseTransitionVFX, effectLocation, effectRotation, FVector(0.9f));
        }
        float transitionDuration = 1.2f;
        if (IsMontageCompatible(m_pPhaseTransitionMontage))
        {
            transitionDuration = FMath::Max(0.6f, PlayAnimMontage(m_pPhaseTransitionMontage));
        }
        if (UCharacterMovementComponent* movementComponent = GetCharacterMovement())
        {
            //第2段階は走行サンプル速度に合わせる
            movementComponent->MaxWalkSpeed = 0.0f;
            movementComponent->StopMovementImmediately();
        }
        TWeakObjectPtr<ABossChara> weakThis = this;
        GetWorldTimerManager().SetTimer(m_phaseTransitionTimer,
                                        FTimerDelegate::CreateLambda(
                                            [weakThis]()
                                            {
                                                if (!weakThis.IsValid() || weakThis->IsDead()) { return; }
                                                weakThis->m_bIsTransitioning = false;
                                                weakThis->m_bAttacking = false;
                                                weakThis->SetCombatActionActive(false);
                                                weakThis->GetCharacterMovement()->MaxWalkSpeed = weakThis->m_chaseRunSpeed;
                                                if (ABossAIController* controller = Cast<ABossAIController>(weakThis->GetController()))
                                                {
                                                    controller->NotifyAttackFinished();
                                                }
                                            }),
                                        transitionDuration, false);
    }

    //ダメージを返す
    return damage;
}

//攻撃を要求する関数
void ABossChara::RequestAttack()
{
    //プレイヤーキャラクターを取得
    APlayerChara* player = GetCombatTarget();

    //ターゲットの位置を決定
    const FVector targetLocation = player ? player->GetActorLocation() : GetActorLocation() + GetActorForwardVector() * m_meleeRange;

    //攻撃パターンを選択して要求
    RequestSpecificAttack(ChoosePattern(), targetLocation);
}

//中ボス/ラスボスでAIプリセットを切り替えます。
EBossAIArchetype ABossChara::GetBossAIArchetype() const { return EBossAIArchetype::Balanced; }

//BossAIControllerが次の行動を選んでよいか確認するための関数
bool ABossChara::CanPerformTacticalAction() const { return !m_bAttacking && !IsDead() && !m_bIsTransitioning && !m_bRecovering; }

//BossAIControllerから指定された攻撃パターンを開始
bool ABossChara::RequestSpecificAttack(EBossAttackPattern _pattern, const FVector& _targetLocation)
{
    //攻撃可能かどうかを確認
    if (!CanPerformTacticalAction() || !SupportsAttackPattern(_pattern)) { return false; }
    //仲間が攻撃中の方向へ割り込まず、空いた間合いから仕掛ける。
    if (const AEnemyAIController* controller = Cast<AEnemyAIController>(GetController()))
    {
        if (controller->HasTarget() && !controller->HasAttackOpening()) { return false; }
    }
    //全パターンで経路移動を止め、モーションと突進だけが位置を決めるようにする
    if (!CanHitPlayer(GetBossAttackRequestRange(), -1.0f)) { return false; }
    SetCombatActionActive(true);
    m_bRecovering = false;
    if (_pattern == EBossAttackPattern::LightCombo || _pattern == EBossAttackPattern::PowerSlam)
    {
        //プレイヤーを保持します。
        const APlayerChara* player = GetCombatTarget();
        if (!player)
        {
            SetCombatActionActive(false);
            return false;
        }
        const float combinedCapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius() + player->GetCapsuleComponent()->GetScaledCapsuleRadius();
        const float effectiveDistance =
            //最大を処理します。
            FMath::Max(0.0f, FVector::Dist2D(GetActorLocation(), player->GetActorLocation()) - combinedCapsuleRadius);
        const float allowedDistance = _pattern == EBossAttackPattern::LightCombo ? GetContactAttackRange() : GetSlamStartRange();
        if (effectiveDistance > allowedDistance)
        {
            SetCombatActionActive(false);
            return false;
        }

        //ライトコンボとパワースラムは定位置攻撃のため、開始前に残留移動を除去します。
        //モンタージュ開始前に経路追従速度を消し、攻撃中の滑りを防ぎます。
        if (ABossAIController* bossController = Cast<ABossAIController>(GetController()))
        {
            bossController->StopMovement();
        }
        if (UCharacterMovementComponent* movementComponent = GetCharacterMovement())
        {
            movementComponent->StopMovementImmediately();
        }
    }

    //ターゲットの方向を向く
    const FVector toTarget = _targetLocation - GetActorLocation();
    if (!toTarget.IsNearlyZero())
    {
        SetActorRotation(FRotator(0.0f, toTarget.Rotation().Yaw, 0.0f));
    }

    //攻撃パターンを設定して実行
    m_currentPattern = _pattern;
    m_requestedTargetLocation = _targetLocation;
    if (AEnemyAIController* controller = Cast<AEnemyAIController>(GetController())) { controller->NotifyAttackStarted(); }
    GetCharacterMovement()->MaxWalkSpeed = 0.0f;
    ExecutePattern(m_currentPattern);
    //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
    return true;
}

//アニメーションがない状態で移動だけが発生しないよう、攻撃資産を検証する
bool ABossChara::SupportsAttackPattern(EBossAttackPattern _pattern) const
{
    switch (_pattern)
    {
    case EBossAttackPattern::LightCombo:
        return IsMontageCompatible(m_pLightComboMontage) ||
               m_comboAttackMontages.ContainsByPredicate([this](const TObjectPtr<UAnimMontage>& _montage) { return IsMontageCompatible(_montage); });
    case EBossAttackPattern::PowerSlam: return IsMontageCompatible(m_pPowerSlamMontage);
    case EBossAttackPattern::ChargeRush: return IsMontageCompatible(m_pChargeRushMontage);
    case EBossAttackPattern::BackStep: return IsMontageCompatible(m_pBackStepMontage);
    default: return false;
    }
}

//現在のボスクラスが指定された戦術行動を使用できるか判定します。
bool ABossChara::SupportsTacticalAction(EBossTacticalAction _action) const
{
    switch (_action)
    {
    case EBossTacticalAction::LightCombo: return SupportsAttackPattern(EBossAttackPattern::LightCombo);
    case EBossTacticalAction::PowerSlam: return SupportsAttackPattern(EBossAttackPattern::PowerSlam);
    case EBossTacticalAction::ChargeRush: return SupportsAttackPattern(EBossAttackPattern::ChargeRush);
    case EBossTacticalAction::BackStep: return SupportsAttackPattern(EBossAttackPattern::BackStep);
    default: return true;
    }
}

//BossAIControllerが行動の優先度を計算するための関数
float ABossChara::ModifyUtilityScore(EBossTacticalAction _action, const FBossDecisionContext& _context, float _baseScore) const { return _baseScore; }

//UtilityAIが距離評価に使う近接攻撃距離
float ABossChara::GetBossMeleeRange() const { return FMath::Max(100.0f, m_meleeRange); }

//接触攻撃は体の外縁、衝撃波は足元中心が原点なので、体の半径分だけ開始距離を縮める。
float ABossChara::GetSlamStartRange() const
{
    const APlayerChara* player = GetCombatTarget();
    const float playerRadius = player ? player->GetCapsuleComponent()->GetScaledCapsuleRadius() : 42.0f;
    return FMath::Max(0.0f, GetBossMeleeRange() - GetCapsuleComponent()->GetScaledCapsuleRadius() - playerRadius);
}

//UtilityAIが攻撃を要求し始める距離
float ABossChara::GetBossAttackRequestRange() const { return FMath::Max(GetBossMeleeRange(), m_chargeDistance); }

//0.0 - 1.0 のHP割合
float ABossChara::GetHealthRatio() const
{
    //最大HPが0以下の場合は1.0を返す
    const float maxHp = GetMaxHP();
    if (maxHp <= 0.0f) { return 1.0f; }

    //HP割合を計算して0.0から1.0の範囲にクランプして返す
    return FMath::Clamp(GetHP() / maxHp, 0.0f, 1.0f);
}

//攻撃判定を実行する関数
void ABossChara::PerformBossAttackHit()
{
    //攻撃中でない場合や死亡している場合は処理を終了
    if (IsDead() || !m_bAttacking || m_bRecovering || m_bIsTransitioning) { return; }
    if (m_currentPattern == EBossAttackPattern::LightCombo)
    {
        ExecuteLightComboAreaAttack();
        return;
    }

    //AM_MutantSlam の衝撃通知に合わせ、地面エフェクトも同じフレームで再生します。
    if (m_currentPattern == EBossAttackPattern::PowerSlam)
    {
        if (m_bSlamHitResolved) { return; }
        m_bSlamHitResolved = true;
        SpawnPowerSlamEffect();
        FVector contact;
        FRotator rotation;
        ResolveGroundEffectTransform(0.0f, contact, rotation);
        APlayerChara* player = GetCombatTarget();
        //地面に接触した瞬間だけ、衝撃波の範囲と遮蔽物を確認してダメージを与える。
        if (player && !player->IsDead() && FVector::Dist2D(contact, player->GetActorLocation()) <= GetBossMeleeRange() &&
            CanHitPlayer(GetBossMeleeRange() * 1.15f, -0.2f))
        {
            UGameplayStatics::ApplyDamage(player, static_cast<float>(m_hitDamage), GetController(), this, UDamageType::StaticClass());
            OnEnemyAttackHit(player);
        }
        return;
    }

    //攻撃判定をリセットして新しいスイングの準備をする
    ResetAttackHitForNewSwing();
    SetAttackCollisionEnabled(true);

    //攻撃判定を短時間有効にした後、無効化するためのタイマーを設定
    TWeakObjectPtr<ABossChara> weakThis = this;
    FTimerHandle localTimerHandle;
    GetWorldTimerManager().SetTimer(localTimerHandle,
                                    FTimerDelegate::CreateLambda(
                                        [weakThis]()
                                        {
                                            if (weakThis.IsValid())
                                            {
                                                weakThis->SetAttackCollisionEnabled(false);
                                            }
                                        }),
                                    0.08f, false);
}

//攻撃判定が有効な時間だけボス攻撃エフェクトを表示します。
void ABossChara::BeginAttackVFXWindow()
{
    if (m_currentPattern == EBossAttackPattern::PowerSlam)
    {
        PerformBossAttackHit();
        return;
    }
    if (m_currentPattern == EBossAttackPattern::LightCombo)
    {
        //足元へ衝撃を出し、ダメージは通知区間の手足の接触だけで与える。
        ExecuteLightComboAreaAttack();
        return;
    }

    //突進と後退の演出は各動作の開始時に接地させているため、手への追従演出は重ねない。
}

void ABossChara::PlayDeathAnimationAndDie()
{
    ClearCombatTimers();
    m_bAttacking = false;
    m_bIsTransitioning = false;
    m_bRecovering = false;
    Super::PlayDeathAnimationAndDie();
}

//死亡処理を行う関数
void ABossChara::Die()
{
    DropBossLoot();
    //共通の撃破通知、通常ドロップ、スポナーの生存数管理を維持します。
    AEnemyChara::Die();
}

//ボスの戦利品をドロップする関数
void ABossChara::DropBossLoot()
{
    {
        if (!GetWorld()) { return; }
        TSubclassOf<APickUpBase> pickupClass = m_pWeaponDropClass;
        if (!pickupClass && GetBossAIArchetype() == EBossAIArchetype::MidBoss)
        {
            pickupClass = LoadClass<APickUpBase>(nullptr, TEXT("/Game/Blueprints/Weapons/BP_PickUp_AR.BP_PickUp_AR_C"));
        }
        if (!pickupClass) { return; }
        FActorSpawnParameters spawnParameters;
        spawnParameters.Owner = this;
        spawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        //拾得物を保持します。
        APickUpBase* pickup =
            GetWorld()->SpawnActor<APickUpBase>(pickupClass, GetActorLocation() + FVector(0.0f, 0.0f, 60.0f), FRotator::ZeroRotator, spawnParameters);
        if (pickup)
        {
            //アイテムを取得し、効果を反映します。
            pickup->PickUpItem(EItemType::EIT_WeaponAR, m_weaponDropValue > 0.0f ? m_weaponDropValue : 120.0f);
        }
    }
}
