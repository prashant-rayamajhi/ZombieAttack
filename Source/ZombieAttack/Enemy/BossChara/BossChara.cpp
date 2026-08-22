#include "BossChara.h"

#include "ZombieAttack/PickUp/PickUpBase.h"
#include "ZombieAttack/Player/PlayerChara.h"
#include "ZombieAttack/AIController/BossAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

//コンストラクタ
ABossChara::ABossChara()
    : m_phaseTwoThreshold(0.5f), m_pLightComboMontage(nullptr), m_pPowerSlamMontage(nullptr), m_pChargeRushMontage(nullptr),
      m_pBackStepMontage(nullptr), m_pPhaseTransitionMontage(nullptr), m_pRoarSound(nullptr), m_pImpactSound(nullptr), m_pWeaponDropClass(nullptr),
      m_weaponDropValue(0.0f), m_attackCooldown(3.0f), m_phaseTwoAttackCooldown(1.0f), m_meleeRange(200.0f), m_chargeDistance(600.0f),
      m_powerSlamForwardOffset(110.0f), m_powerSlamEffectLeadTime(0.18f), m_lightComboEffectRadius(175.0f), m_lightComboVerticalTolerance(160.0f),
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
    m_MaxHp = 500.0f;
    m_Hp = m_MaxHp;
    OnHealthChanged.Broadcast();
}

//毎フレーム呼ばれる関数
void ABossChara::Tick(float _deltaTime)
{
    //フレームごとの経過時間を使って、移動や表示の変化を更新します。
    Super::Tick(_deltaTime);
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
    //「IsDead()」が成立するとき、続けて「!m_bPhaseTwo && GetHealthRatio() <= m_phaseTwoThreshold」を判定します。
    if (IsDead()) { return damage; }

    //フェーズ2への移行条件をチェック
    if (!m_bPhaseTwo && GetHealthRatio() <= m_phaseTwoThreshold)
    {
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

        //「m_pPhaseTransitionVFX」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
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

        //transitionDurationは、1.2fから算出した数値を後続の判定または計算に使います。
        float transitionDuration = 1.2f;
        //「m_pPhaseTransitionMontage」が成立するとき、Maxを呼び出します。
        if (m_pPhaseTransitionMontage)
        {
            transitionDuration = FMath::Max(0.6f, PlayAnimMontage(m_pPhaseTransitionMontage));
        }

        //「UCharacterMovementComponent* movementComponent = GetCharacterMovement()」が成立するとき、movementComponent->MaxWalkSpeedを更新します。
        if (UCharacterMovementComponent* movementComponent = GetCharacterMovement())
        {
            //第2段階は走行サンプル速度に合わせる
            movementComponent->MaxWalkSpeed = 500.0f;
            movementComponent->StopMovementImmediately();
        }

        //weakThisは、thisから構築した結果を後続の処理へ渡すために使います。
        TWeakObjectPtr<ABossChara> weakThis = this;
        GetWorldTimerManager().SetTimer(m_phaseTransitionTimer,
                                        FTimerDelegate::CreateLambda(
                                            [weakThis]()
                                            {
                                                //「!weakThis.IsValid()」が成立するとき、weakThis->m_bIsTransitioningを更新します。
                                                if (!weakThis.IsValid()) { return; }
                                                weakThis->m_bIsTransitioning = false;
                                                weakThis->m_bAttacking = false;
                                                //「ABossAIController* controller」が成立するとき、GetControllerを呼び出します。
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
    APlayerChara* player = Cast<APlayerChara>(UGameplayStatics::GetPlayerCharacter(this, 0));

    //ターゲットの位置を決定
    const FVector targetLocation = player ? player->GetActorLocation() : GetActorLocation() + GetActorForwardVector() * m_meleeRange;

    //攻撃パターンを選択して要求
    RequestSpecificAttack(ChoosePattern(), targetLocation);
}

//中ボス/ラスボスでAIプリセットを切り替えます。
EBossAIArchetype ABossChara::GetBossAIArchetype() const { return EBossAIArchetype::Balanced; }

//BossAIControllerが次の行動を選んでよいか確認するための関数
bool ABossChara::CanPerformTacticalAction() const { return !m_bAttacking && !IsDead() && !m_bIsTransitioning; }

//BossAIControllerから指定された攻撃パターンを開始
bool ABossChara::RequestSpecificAttack(EBossAttackPattern _pattern, const FVector& _targetLocation)
{
    //攻撃可能かどうかを確認
    if (!CanPerformTacticalAction() || !SupportsAttackPattern(_pattern)) { return false; }

    //「_pattern == EBossAttackPattern::LightCombo || _pattern == EBossAttackPattern::PowerSlam」が成立するとき、GetPlayerCharacterを呼び出します。
    if (_pattern == EBossAttackPattern::LightCombo || _pattern == EBossAttackPattern::PowerSlam)
    {
        //プレイヤーを保持します。
        const APlayerChara* player = Cast<APlayerChara>(UGameplayStatics::GetPlayerCharacter(this, 0));
        //「!player」が成立するとき、GetCapsuleComponentを呼び出します。
        if (!player) { return false; }

        //combinedCapsuleRadiusは、GetCapsuleComponent()->GetScaledCapsuleRadius() + player->GetCapsuleCom…から算出した数値を後続の判定または計算に使います。
        const float combinedCapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius() + player->GetCapsuleComponent()->GetScaledCapsuleRadius();
        //effectiveDistanceは、ゲーム判定に使用する数値を計算し、後続の比較または更新へ渡すために使います。
        const float effectiveDistance =
            //最大を処理します。
            FMath::Max(0.0f, FVector::Dist2D(GetActorLocation(), player->GetActorLocation()) - combinedCapsuleRadius);
        //allowedDistanceは、_pattern == EBossAttackPattern::LightCombo ? GetBossMeleeRange() : GetB…から算出した数値を後続の判定または計算に使います。
        const float allowedDistance = _pattern == EBossAttackPattern::LightCombo ? GetBossMeleeRange() : GetBossMeleeRange() * 1.15f;
        //「effectiveDistance > allowedDistance」が成立するとき、続けて「ABossAIController* bossController = Cast<ABossAIController>(GetContro…」を判定します。
        if (effectiveDistance > allowedDistance) { return false; }

        //ライトコンボとパワースラムは定位置攻撃のため、開始前に残留移動を除去します。
        //モンタージュ開始前に経路追従速度を消し、攻撃中の滑りを防ぎます。
        if (ABossAIController* bossController = Cast<ABossAIController>(GetController()))
        {
            bossController->StopMovement();
        }
        //「UCharacterMovementComponent* movementComponent = GetCharacterMovement()」が成立するとき、StopMovementImmediatelyを呼び出します。
        if (UCharacterMovementComponent* movementComponent = GetCharacterMovement())
        {
            movementComponent->StopMovementImmediately();
        }
    }

    //ターゲットの方向を向く
    const FVector toTarget = _targetLocation - GetActorLocation();
    //「!toTarget.IsNearlyZero()」が成立するとき、SetActorRotationを呼び出します。
    if (!toTarget.IsNearlyZero())
    {
        SetActorRotation(FRotator(0.0f, toTarget.Rotation().Yaw, 0.0f));
    }

    //攻撃パターンを設定して実行
    m_currentPattern = _pattern;
    m_requestedTargetLocation = _targetLocation;
    ExecutePattern(m_currentPattern);
    //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
    return true;
}

//アニメーションがない状態で移動だけが発生しないよう、攻撃資産を検証する
bool ABossChara::SupportsAttackPattern(EBossAttackPattern _pattern) const
{
    //現在の状態に合う処理へ分けます。
    switch (_pattern)
    {
    case EBossAttackPattern::LightCombo:
        return IsMontageCompatible(m_pLightComboMontage) ||
               m_comboAttackMontages.ContainsByPredicate([this](const TObjectPtr<UAnimMontage>& _montage) { return IsMontageCompatible(_montage); });
    //IsMontageCompatibleは、名前が示す条件の成立可否を呼び出し元へ返します。
    case EBossAttackPattern::PowerSlam: return IsMontageCompatible(m_pPowerSlamMontage);
    //IsMontageCompatibleは、名前が示す条件の成立可否を呼び出し元へ返します。
    case EBossAttackPattern::ChargeRush: return IsMontageCompatible(m_pChargeRushMontage);
    //IsMontageCompatibleは、名前が示す条件の成立可否を呼び出し元へ返します。
    case EBossAttackPattern::BackStep: return IsMontageCompatible(m_pBackStepMontage);
    default: return false;
    }
}

//現在のボスクラスが指定された戦術行動を使用できるか判定します。
bool ABossChara::SupportsTacticalAction(EBossTacticalAction _action) const
{
    //現在の状態に合う処理へ分けます。
    switch (_action)
    {
    //SupportsAttackPatternは、SupportsAttackPatternの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    case EBossTacticalAction::LightCombo: return SupportsAttackPattern(EBossAttackPattern::LightCombo);
    //SupportsAttackPatternは、SupportsAttackPatternの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    case EBossTacticalAction::PowerSlam: return SupportsAttackPattern(EBossAttackPattern::PowerSlam);
    //SupportsAttackPatternは、SupportsAttackPatternの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    case EBossTacticalAction::ChargeRush: return SupportsAttackPattern(EBossAttackPattern::ChargeRush);
    //SupportsAttackPatternは、SupportsAttackPatternの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    case EBossTacticalAction::BackStep: return SupportsAttackPattern(EBossAttackPattern::BackStep);
    default: return true;
    }
}

//BossAIControllerが行動の優先度を計算するための関数
float ABossChara::ModifyUtilityScore(EBossTacticalAction _action, const FBossDecisionContext& _context, float _baseScore) const { return _baseScore; }

//UtilityAIが距離評価に使う近接攻撃距離
float ABossChara::GetBossMeleeRange() const { return FMath::Max(100.0f, m_meleeRange); }

//UtilityAIが攻撃を要求し始める距離
float ABossChara::GetBossAttackRequestRange() const { return FMath::Max(GetBossMeleeRange(), m_chargeDistance); }

//0.0 - 1.0 のHP割合
float ABossChara::GetHealthRatio() const
{
    //最大HPが0以下の場合は1.0を返す
    const float maxHp = GetMaxHP();
    //「maxHp <= 0.0f」が成立するとき、この関数を終了します。
    if (maxHp <= 0.0f) { return 1.0f; }

    //HP割合を計算して0.0から1.0の範囲にクランプして返す
    return FMath::Clamp(GetHP() / maxHp, 0.0f, 1.0f);
}

//攻撃判定を実行する関数
void ABossChara::PerformBossAttackHit()
{
    //攻撃中でない場合や死亡している場合は処理を終了
    if (IsDead()) { return; }

    //「m_currentPattern == EBossAttackPattern::LightCombo」が成立するとき、ExecuteLightComboAreaAttackを呼び出します。
    if (m_currentPattern == EBossAttackPattern::LightCombo)
    {
        ExecuteLightComboAreaAttack();
        return;
    }

    //AM_MutantSlam の衝撃通知に合わせ、地面エフェクトも同じフレームで再生します。
    if (m_currentPattern == EBossAttackPattern::PowerSlam)
    {
        SpawnPowerSlamEffect();
    }

    //攻撃判定をリセットして新しいスイングの準備をする
    ResetAttackHitForNewSwing();
    SetAttackCollisionEnabled(true);

    //攻撃判定を短時間有効にした後、無効化するためのタイマーを設定
    TWeakObjectPtr<ABossChara> weakThis = this;
    //localTimerHandleは、タイマーの登録と解除を同じハンドルで管理するために使います。
    FTimerHandle localTimerHandle;
    GetWorldTimerManager().SetTimer(localTimerHandle,
                                    FTimerDelegate::CreateLambda(
                                        [weakThis]()
                                        {
                                            //「weakThis.IsValid()」が成立するとき、SetAttackCollisionEnabledを呼び出します。
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
    //「m_currentPattern == EBossAttackPattern::LightCombo」が成立するとき、ExecuteLightComboAreaAttackを呼び出します。
    if (m_currentPattern == EBossAttackPattern::LightCombo)
    {
        //パンチは手元の円形VFXと同じ範囲で判定し、Boxとの二重ヒットを防ぎます。
        ExecuteLightComboAreaAttack();
        //通常攻撃はNotifyの瞬間に円形範囲で判定するため、旧Box判定を併用しません。
        SetAttackCollisionEnabled(false);
        return;
    }

    //攻撃判定が有効な時間だけボス攻撃エフェクトを表示します。
    Super::BeginAttackVFXWindow();
}

//攻撃パターンを選択する関数
EBossAttackPattern ABossChara::ChoosePattern() const
{
    //ランダムな値を生成して攻撃パターンを選択
    const float randomValue = FMath::FRand();

    //フェーズ2の場合、攻撃パターンの確率を変更
    if (m_bPhaseTwo)
    {
        //「randomValue < 0.45f」が成立するとき、続けて「randomValue < 0.70f」を判定します。
        if (randomValue < 0.45f) { return EBossAttackPattern::LightCombo; }
        //「randomValue < 0.70f」が成立するとき、続けて「randomValue < 0.90f」を判定します。
        if (randomValue < 0.70f) { return EBossAttackPattern::PowerSlam; }
        //「randomValue < 0.90f」が成立するとき、この関数を終了します。
        if (randomValue < 0.90f) { return EBossAttackPattern::ChargeRush; }
        return EBossAttackPattern::BackStep;
    }

    //フェーズ1の場合、攻撃パターンの確率を設定
    if (randomValue < 0.45f) { return EBossAttackPattern::LightCombo; }
    //「randomValue < 0.65f」が成立するとき、続けて「randomValue < 0.85f」を判定します。
    if (randomValue < 0.65f) { return EBossAttackPattern::PowerSlam; }
    //「randomValue < 0.85f」が成立するとき、この関数を終了します。
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
    //montageToPlayは、nullptrから取得した参照を後続の呼び出しで使います。
    UAnimMontage* montageToPlay = nullptr;

    //コンボ攻撃の現在のステップに対応するアニメーションモンタージュを取得
    if (m_comboAttackMontages.IsValidIndex(m_currentComboIndex))
    {
        montageToPlay = m_comboAttackMontages[m_currentComboIndex];
    }
    //「!IsMontageCompatible(montageToPlay)」が成立するとき、montageToPlayを更新します。
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
        //montageDurationは、PlayAnimMontage(montageToPlay)から算出した数値を後続の判定または計算に使います。
        const float montageDuration = PlayAnimMontage(montageToPlay);
        if (montageDuration <= 0.0f)
        {
            FinishBossAttack();
            return;
        }

        GetWorldTimerManager().ClearTimer(m_lightComboEffectTimer);
        GetWorldTimerManager().SetTimer(m_lightComboEffectTimer, this, &ABossChara::ExecuteLightComboAreaAttack,
                                        FMath::Max(0.05f, montageDuration * 0.55f), false);

        //weakThisは、thisから構築した結果を後続の処理へ渡すために使います。
        TWeakObjectPtr<ABossChara> weakThis = this;
        GetWorldTimerManager().SetTimer(m_attackAnimationSafetyTimer,
                                        FTimerDelegate::CreateLambda(
                                            [weakThis]()
                                            {
                                                //「weakThis.IsValid() && weakThis->m_bAttacking」が成立するとき、FinishBossAttackを呼び出します。
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

//通常攻撃の手元に見える円形範囲と、同じ半径のダメージ判定を生成します。
void ABossChara::ExecuteLightComboAreaAttack()
{
    //「m_bLightComboAreaResolvedThisStep || IsDead()」が成立するとき、m_bLightComboAreaResolvedThisStepを更新します。
    if (m_bLightComboAreaResolvedThisStep || IsDead()) { return; }
    m_bLightComboAreaResolvedThisStep = true;

    //メッシュを返します。
    const USkeletalMeshComponent* bossMesh = GetMesh();
    const FVector handLocation =
        bossMesh && bossMesh->DoesSocketExist(TEXT("hand_r"))
            ? bossMesh->GetSocketLocation(TEXT("hand_r"))
            : (m_pAttackCollision ? m_pAttackCollision->GetComponentLocation() : GetActorLocation() + GetActorForwardVector() * 90.0f);

    //comboImpactVFXは、m_pComboImpactVFX.Get()から取得した参照を後続の呼び出しで使います。
    UNiagaraSystem* comboImpactVFX = m_pComboImpactVFX.Get();
    //「!comboImpactVFX」が成立するとき、comboImpactVFXを更新します。
    if (!comboImpactVFX)
    {
        //BP側が未設定でも、作品内の既定エフェクトで見た目が欠落しないようにします。
        comboImpactVFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave"));
    }

    //「comboImpactVFX」が成立するとき、SpawnSystemAtLocationを呼び出します。
    if (comboImpactVFX)
    {
        //SystemAt位置を作成します。
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, comboImpactVFX, handLocation, GetActorRotation(), FVector(0.20f));
    }

    //プレイヤーキャラクターを返します。
    APlayerChara* player = Cast<APlayerChara>(UGameplayStatics::GetPlayerCharacter(this, 0));
    //「!player || player->IsDead()」が成立するとき、GetActorLocationを呼び出します。
    if (!player || player->IsDead()) { return; }

    //toPlayerは、player->GetActorLocation() - handLocationから取得した参照を後続の呼び出しで使います。
    const FVector toPlayer = player->GetActorLocation() - handLocation;
    //プレイヤー半径を保持します。
    const float playerRadius = player->GetCapsuleComponent()->GetScaledCapsuleRadius();
    //プレイヤーが攻撃範囲の横幅内にいるかを示します。
    const bool bInsideHorizontalRange = toPlayer.Size2D() <= m_lightComboEffectRadius + playerRadius;
    //プレイヤーとの高低差が攻撃可能範囲内かを示します。
    const bool bInsideVerticalRange = FMath::Abs(toPlayer.Z) <= m_lightComboVerticalTolerance;
    //「!bInsideHorizontalRange || !bInsideVerticalRange」が成立するとき、ApplyDamageを呼び出します。
    if (!bInsideHorizontalRange || !bInsideVerticalRange) { return; }

    //ダメージを処理します。
    UGameplayStatics::ApplyDamage(player, static_cast<float>(m_hitDamage), GetController(), this, UDamageType::StaticClass());
    OnEnemyAttackHit(player);
}

//攻撃がヒットしたときの処理
void ABossChara::OnEnemyAttackHit(AActor* _hitActor)
{
    //「!_hitActor」が成立するとき、Getを呼び出します。
    if (!_hitActor) { return; }

    //命中した瞬間だけ、攻撃種類に合うインパクトを表示する
    //Slamは接地Notifyで生成済みのため、プレイヤー位置へ二重生成しません。
    UNiagaraSystem* impactSystem = m_currentPattern == EBossAttackPattern::PowerSlam ? nullptr : m_pComboImpactVFX.Get();
    //「impactSystem」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (impactSystem)
    {
        //エフェクト位置を保持します。
        FVector effectLocation;
        //エフェクト回転を保持します。
        FRotator effectRotation;
        ResolveGroundEffectTransformAt(_hitActor->GetActorLocation(), effectLocation, effectRotation);
        //effectScaleは、m_currentPattern == EBossAttackPattern::PowerSlam ? 0.52f : 0.14fから算出した数値を後続の判定または計算に使います。
        const float effectScale = m_currentPattern == EBossAttackPattern::PowerSlam ? 0.52f : 0.14f;
        //SystemAt位置を作成します。
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, impactSystem, effectLocation, effectRotation, FVector(effectScale));
    }
    m_bComboHitConfirmed = true;

    //ヒット時のサウンドを再生
    if (m_pImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, m_pImpactSound, GetActorLocation());
    }

    //BossAIControllerにプレイヤーがヒットしたことを通知
    if (ABossAIController* bossAIController = Cast<ABossAIController>(GetController()))
    {
        bossAIController->NotifyPlayerHit(1.0f);
    }
}

//コンボ攻撃の分岐を要求する関数
void ABossChara::RequestComboBranchFromNotify()
{
    //攻撃判定を無効化
    SetAttackCollisionEnabled(false);

    //コンボ攻撃がライトコンボでない場合、攻撃を終了
    if (m_currentPattern != EBossAttackPattern::LightCombo)
    {
        FinishBossAttack();
        return;
    }

    //コンボ攻撃がヒットしていない場合、攻撃を終了
    if (!m_bComboHitConfirmed)
    {
        FinishBossAttack();
        return;
    }

    //次のコンボステップのインデックスを計算
    const int32 nextComboIndex = m_currentComboIndex + 1;
    //「!m_comboAttackMontages.IsValidIndex(nextComboIndex)」が成立するとき、FinishBossAttackを呼び出します。
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

    //パワースラムのアニメーションモンタージュを再生するか、攻撃を終了する
    if (IsMontageCompatible(m_pPowerSlamMontage))
    {
        //montageDurationは、PlayAnimMontage(m_pPowerSlamMontage)から算出した数値を後続の判定または計算に使います。
        const float montageDuration = PlayAnimMontage(m_pPowerSlamMontage);
        //「montageDuration <= 0.0f」が成立するとき、FinishBossAttackを呼び出します。
        if (montageDuration <= 0.0f)
        {
            FinishBossAttack();
            return;
        }

        //ダメージNotifyとは分離し、モンタージュ終盤で地面の衝撃波だけを生成します。
        GetWorldTimerManager().ClearTimer(m_powerSlamEffectTimer);
        GetWorldTimerManager().SetTimer(m_powerSlamEffectTimer, this, &ABossChara::SpawnPowerSlamEffect,
                                        FMath::Max(0.01f, montageDuration - m_powerSlamEffectLeadTime), false);

        //weakThisは、thisから構築した結果を後続の処理へ渡すために使います。
        TWeakObjectPtr<ABossChara> weakThis = this;
        GetWorldTimerManager().SetTimer(m_attackAnimationSafetyTimer,
                                        FTimerDelegate::CreateLambda(
                                            [weakThis]()
                                            {
                                                //「weakThis.IsValid()」が成立するとき、FinishBossAttackを呼び出します。
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

//Slamの演出だけをモンタージュ終盤に生成します。
void ABossChara::SpawnPowerSlamEffect()
{
    //死亡状態かどうかを返します。
    if (!m_bAttacking || IsDead() || m_currentPattern != EBossAttackPattern::PowerSlam || m_bPowerSlamEffectSpawned) { return; }

    //powerSlamVFXは、m_pPowerSlamVFX.Get()から取得した参照を後続の呼び出しで使います。
    UNiagaraSystem* powerSlamVFX = m_pPowerSlamVFX.Get();
    //「!powerSlamVFX」が成立するとき、powerSlamVFXを更新します。
    if (!powerSlamVFX)
    {
        //BPに参照が保存されていなくても、既定の衝撃波を使用します。
        powerSlamVFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave"));
    }
    if (!powerSlamVFX)
    {
        return;
    }

    //エフェクト位置を保持します。
    FVector effectLocation;
    //エフェクト回転を保持します。
    FRotator effectRotation;
    ResolveGroundEffectTransform(m_powerSlamForwardOffset, effectLocation, effectRotation);

    //SystemAt位置を作成します。
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, powerSlamVFX, effectLocation, effectRotation, FVector(0.52f));

    m_bPowerSlamEffectSpawned = true;
}

//ボスの足元または指定した前方位置から、実際の地面Transformを取得します。
bool ABossChara::ResolveGroundEffectTransform(float _forwardOffset, FVector& _outLocation, FRotator& _outRotation) const
{
    //traceCenterは、GetActorLocation() + GetActorForwardVector() * FMath::Max(0.0f, _forwar…から求めた空間情報を位置または向きの計算に使います。
    const FVector traceCenter = GetActorLocation() + GetActorForwardVector() * FMath::Max(0.0f, _forwardOffset);
    //ResolveGroundEffectTransformAtは、ResolveGroundEffectTransformAtの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    return ResolveGroundEffectTransformAt(traceCenter, _outLocation, _outRotation);
}

//地面をTraceし、攻撃エフェクトの位置と傾きを接地面へ合わせます。
bool ABossChara::ResolveGroundEffectTransformAt(const FVector& _traceCenter, FVector& _outLocation, FRotator& _outRotation) const
{
    //groundHitは、衝突判定の結果を受け取り、命中位置や対象を参照するために使います。
    FHitResult groundHit;
    //queryParamsは、queryParamsの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    FCollisionQueryParams queryParams(SCENE_QUERY_STAT(BossGroundEffect), false, this);
    queryParams.AddIgnoredActor(this);

    //ワールドを返します。
    const bool bFoundGround =
        GetWorld() && GetWorld()->LineTraceSingleByChannel(groundHit, _traceCenter + FVector(0.0f, 0.0f, 260.0f),
                                                           _traceCenter - FVector(0.0f, 0.0f, 500.0f), ECC_WorldStatic, queryParams);

    //fallbackGroundZは、GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeigh…から算出した数値を後続の判定または計算に使います。
    const float fallbackGroundZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    _outLocation = bFoundGround ? groundHit.ImpactPoint + groundHit.ImpactNormal * 3.0f : FVector(_traceCenter.X, _traceCenter.Y, fallbackGroundZ);
    _outRotation = bFoundGround ? FRotationMatrix::MakeFromZ(groundHit.ImpactNormal).Rotator() : FRotator::ZeroRotator;
    //bFoundGroundは、判定結果を保持し、直後の条件分岐で実行可否を決めるために使います。
    return bFoundGround;
}

//チャージラッシュ攻撃を実行する関数
void ABossChara::DoChargeRush()
{
    //プレイヤーキャラクターを取得
    APlayerChara* player = Cast<APlayerChara>(UGameplayStatics::GetPlayerCharacter(this, 0));
    //「!player」が成立するとき、FinishBossAttackを呼び出します。
    if (!player)
    {
        FinishBossAttack();
        return;
    }

    //「!IsMontageCompatible(m_pChargeRushMontage)」が成立するとき、FinishBossAttackを呼び出します。
    if (!IsMontageCompatible(m_pChargeRushMontage))
    {
        FinishBossAttack();
        return;
    }
    //montageDurationは、PlayAnimMontage(m_pChargeRushMontage)から算出した数値を後続の判定または計算に使います。
    const float montageDuration = PlayAnimMontage(m_pChargeRushMontage);
    //「montageDuration <= 0.0f」が成立するとき、FinishBossAttackを呼び出します。
    if (montageDuration <= 0.0f)
    {
        FinishBossAttack();
        return;
    }

    //「m_pChargeVFX」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
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
    const FVector direction = (targetLocation - GetActorLocation()).GetSafeNormal2D();
    SetActorRotation(FRotator(0.0f, direction.Rotation().Yaw, 0.0f));
    //「GetCharacterMovement()」が成立するとき、Maxを呼び出します。
    if (GetCharacterMovement())
    {
        //horizontalSpeedは、m_chargeDistance / FMath::Max(0.2f, montageDuration)から算出した数値を後続の判定または計算に使います。
        const float horizontalSpeed = m_chargeDistance / FMath::Max(0.2f, montageDuration);
        LaunchCharacter(direction * horizontalSpeed, true, true);
    }

    //weakThisは、thisから構築した結果を後続の処理へ渡すために使います。
    TWeakObjectPtr<ABossChara> weakThis = this;
    GetWorldTimerManager().SetTimer(m_chargeTimer,
                                    FTimerDelegate::CreateLambda(
                                        [weakThis]()
                                        {
                                            //「weakThis.IsValid()」が成立するとき、FinishBossAttackを呼び出します。
                                            if (weakThis.IsValid())
                                            {
                                                weakThis->FinishBossAttack();
                                            }
                                        }),
                                    FMath::Max(0.2f, montageDuration), false);
}

//バックステップ攻撃を実行する関数
void ABossChara::DoBackStep()
{
    //「!IsMontageCompatible(m_pBackStepMontage)」が成立するとき、FinishBossAttackを呼び出します。
    if (!IsMontageCompatible(m_pBackStepMontage))
    {
        FinishBossAttack();
        return;
    }
    //「PlayAnimMontage(m_pBackStepMontage) <= 0.0f」が成立するとき、FinishBossAttackを呼び出します。
    if (PlayAnimMontage(m_pBackStepMontage) <= 0.0f)
    {
        FinishBossAttack();
        return;
    }
    //「m_pBackStepVFX」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
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
    const FVector backward = -GetActorForwardVector() * m_chargeDistance * 0.5f;
    //「UCharacterMovementComponent* movementComponent = GetCharacterMovement()」が成立するとき、AddImpulseを呼び出します。
    if (UCharacterMovementComponent* movementComponent = GetCharacterMovement())
    {
        movementComponent->AddImpulse(backward * 80000.0f, true);
    }

    //攻撃を終了する
    FinishBossAttack();
}

//攻撃を終了する関数
void ABossChara::FinishBossAttack()
{
    //AM_MutantSlam側の終了通知が早くても、最後の地面衝撃だけは欠落させません。
    if (m_bAttacking && m_currentPattern == EBossAttackPattern::PowerSlam && !m_bPowerSlamEffectSpawned)
    {
        SpawnPowerSlamEffect();
    }

    GetWorldTimerManager().ClearTimer(m_chargeTimer);
    GetWorldTimerManager().ClearTimer(m_powerSlamEffectTimer);
    GetWorldTimerManager().ClearTimer(m_lightComboEffectTimer);
    GetWorldTimerManager().ClearTimer(m_attackAnimationSafetyTimer);
    //攻撃中フラグを解除
    SetAttackCollisionEnabled(false);
    EndAttackVFXWindow();
    //キャラクター移動を返します。
    if (m_currentPattern == EBossAttackPattern::ChargeRush && GetCharacterMovement())
    {
        GetCharacterMovement()->StopMovementImmediately();
    }

    //攻撃クールダウンタイマーを設定
    const float cooldown = m_bPhaseTwo ? m_phaseTwoAttackCooldown : m_attackCooldown;
    //weakThisは、thisから構築した結果を後続の処理へ渡すために使います。
    TWeakObjectPtr<ABossChara> weakThis = this;

    //クールダウン後に攻撃状態をリセットし、BossAIControllerに通知
    GetWorldTimerManager().SetTimer(m_attackCooldownTimer,
                                    FTimerDelegate::CreateLambda(
                                        [weakThis]()
                                        {
                                            //「!weakThis.IsValid()」が成立するとき、weakThis->m_bAttackingを更新します。
                                            if (!weakThis.IsValid()) { return; }

                                            weakThis->m_bAttacking = false;
                                            weakThis->m_currentComboIndex = 0;
                                            weakThis->m_bComboHitConfirmed = false;
                                            weakThis->m_bLightComboAreaResolvedThisStep = false;

                                            //「ABossAIController* bossAIController」が成立するとき、GetControllerを呼び出します。
                                            if (ABossAIController* bossAIController = Cast<ABossAIController>(weakThis->GetController()))
                                            {
                                                bossAIController->NotifyAttackFinished();
                                            }
                                        }),
                                    FMath::Max(0.0f, cooldown), false);
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
        //「!GetWorld()」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
        if (!GetWorld()) { return; }

        //pickupClassは、m_pWeaponDropClassから構築した結果を後続の処理へ渡すために使います。
        TSubclassOf<APickUpBase> pickupClass = m_pWeaponDropClass;
        //「!pickupClass && GetBossAIArchetype() == EBossAIArchetype::MidBoss」が成立するとき、pickupClassを更新します。
        if (!pickupClass && GetBossAIArchetype() == EBossAIArchetype::MidBoss)
        {
            pickupClass = LoadClass<APickUpBase>(nullptr, TEXT("/Game/Blueprints/Weapons/BP_PickUp_AR.BP_PickUp_AR_C"));
        }
        //「!pickupClass」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
        if (!pickupClass) { return; }

        //spawnParametersは、Actor生成時の所有者や衝突時の生成規則を指定するために使います。
        FActorSpawnParameters spawnParameters;
        spawnParameters.Owner = this;
        spawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        //拾得物を保持します。
        APickUpBase* pickup =
            GetWorld()->SpawnActor<APickUpBase>(pickupClass, GetActorLocation() + FVector(0.0f, 0.0f, 60.0f), FRotator::ZeroRotator, spawnParameters);
        //「pickup」が成立するとき、PickUpItemを呼び出します。
        if (pickup)
        {
            //アイテムを取得し、効果を反映します。
            pickup->PickUpItem(EItemType::EIT_WeaponAR, m_weaponDropValue > 0.0f ? m_weaponDropValue : 120.0f);
        }
    }
}
