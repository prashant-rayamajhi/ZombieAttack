#include "BossUtilityAIComponent.h"
#include "ZombieAttack/Enemy/BossChara/BossChara.h"
#include "ZombieAttack/Player/PlayerChara.h"
#include "Components/CapsuleComponent.h"

//コンストラクタ
UBossUtilityAIComponent::UBossUtilityAIComponent()
    : m_adaptationRate(0.16f), m_randomness(0.18f), m_repeatPenalty(0.34f), m_historyLength(4), m_predictionSeconds(0.45f), m_rangedPressure(0.f),
      m_mobilityPressure(0.f), m_vulnerabilityPressure(0.f), m_recentBossDamagePressure(0.f), m_recentSuccessfulHitPressure(0.f),
      m_lastObservationTime(0.f)
{
    //Tickを無効化
    PrimaryComponentTick.bCanEverTick = false;
}

//アーキタイプに応じてパラメータを設定
void UBossUtilityAIComponent::ConfigureForArchetype(EBossAIArchetype _archetype)
{
    //アーキタイプに応じてパラメータを設定
    switch (_archetype)
    {
    //中ボスのアーキタイプに応じてパラメータを設定
    case EBossAIArchetype::MidBoss:
    {
        m_adaptationRate = 0.18f;
        m_randomness = 0.09f;
        m_repeatPenalty = 0.52f;
        m_historyLength = 5;
        m_predictionSeconds = 0.48f;
        break;
    }
    //最終ボスのアーキタイプに応じてパラメータを設定
    case EBossAIArchetype::FinalBoss:
    {
        m_adaptationRate = 0.27f;
        m_randomness = 0.08f;
        m_repeatPenalty = 0.58f;
        m_historyLength = 6;
        m_predictionSeconds = 0.72f;
        break;
    }
    //バランス型のアーキタイプに応じてパラメータを設定
    default:
    {
        m_adaptationRate = 0.16f;
        m_randomness = 0.18f;
        m_repeatPenalty = 0.34f;
        m_historyLength = 4;
        m_predictionSeconds = 0.45f;
        break;
    }
    }

    //履歴をリセット
    m_actionHistory.Reset();
    m_rangedPressure = 0.f;
    m_mobilityPressure = 0.f;
    m_vulnerabilityPressure = 0.f;
    m_recentBossDamagePressure = 0.f;
    m_recentSuccessfulHitPressure = 0.f;
    m_lastObservationTime = 0.f;
}

//一時的な圧力を減衰させる
void UBossUtilityAIComponent::DecayTransientPressure(float _deltaSeconds)
{
    //減衰率を計算
    const float damageDecay = FMath::Exp(-FMath::Max(0.f, _deltaSeconds) * 0.85f);
    const float hitDecay = FMath::Exp(-FMath::Max(0.f, _deltaSeconds) * 0.65f);
    m_recentBossDamagePressure *= damageDecay;
    m_recentSuccessfulHitPressure *= hitDecay;
}

//DecisionContextを作成します。
FBossDecisionContext UBossUtilityAIComponent::BuildDecisionContext(ABossChara* _boss, APlayerChara* _player, bool _bHasLineOfSight)
{
    //ボスキャラクターとプレイヤーキャラクターを取得
    FBossDecisionContext context;
    if (!_boss || !_player) { return context; }

    //現在の時間を取得し、前回の観察時間との差分を計算
    const float currentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    const float deltaSeconds = m_lastObservationTime > 0.f ? FMath::Max(0.f, currentTime - m_lastObservationTime) : 0.f;

    //前回の観察時間を更新し、一時的な圧力を減衰させる
    m_lastObservationTime = currentTime;
    DecayTransientPressure(deltaSeconds);

    //ボスとプレイヤーの位置、速度、方向を取得
    const FVector bossLocation = _boss->GetActorLocation();
    //プレイヤー位置を保持します。
    const FVector playerLocation = _player->GetActorLocation();
    //速度を返します。
    const FVector playerVelocity = _player->GetVelocity();
    const FVector toBoss = (bossLocation - playerLocation).GetSafeNormal2D();
    //プレイヤー方向を保持します。
    const FVector playerDirection = playerVelocity.GetSafeNormal2D();
    const FVector lateral(-toBoss.Y, toBoss.X, 0.f);
    const float directionDot = FVector::DotProduct(playerDirection, toBoss);

    //プレイヤーの行動に基づいて圧力を計算
    const float rangedSample = (_player->IsAiming() ? 0.75f : 0.f) + (_player->GetCurrentSlot() != EWeaponSlot::Knife ? 0.25f : 0.f);
    const float mobilitySample = FMath::Clamp(playerVelocity.Size2D() / 650.f, 0.f, 1.f);
    //回復中かどうかを返します。
    const float vulnerabilitySample =
        (_player->IsHealing() ? 1.f : 0.f) + (_player->IsReloading() ? 0.8f : 0.f) + (_player->IsSwitchingWeapon() ? 0.35f : 0.f);

    //圧力を補間して更新
    const float alpha = FMath::Clamp(m_adaptationRate, 0.01f, 1.f);
    m_rangedPressure = FMath::Lerp(m_rangedPressure, FMath::Clamp(rangedSample, 0.f, 1.f), alpha);
    m_mobilityPressure = FMath::Lerp(m_mobilityPressure, mobilitySample, alpha);
    m_vulnerabilityPressure = FMath::Lerp(m_vulnerabilityPressure, FMath::Clamp(vulnerabilitySample, 0.f, 1.f), alpha);

    //コンテキストを構築
    //大型ボスのカプセル半径を除いた実距離を使い、攻撃可否が早過ぎるタイミングで変化することを防ぎます。
    const float combinedCapsuleRadius =
        _boss->GetCapsuleComponent()->GetScaledCapsuleRadius() + _player->GetCapsuleComponent()->GetScaledCapsuleRadius();
    context.m_distanceToPlayer = FMath::Max(0.0f, FVector::Dist2D(bossLocation, playerLocation) - combinedCapsuleRadius);
    context.m_bossHealthRatio = _boss->GetHealthRatio();
    context.m_playerHealthRatio = _player->GetMaxHP() > 0.f
                                    //最大体力を返します。
                                    ? FMath::Clamp(_player->GetHP() / _player->GetMaxHP(), 0.f, 1.f)
                                    : 0.f;
    context.m_playerSpeed = playerVelocity.Size2D();
    context.m_playerLateralSpeed = FMath::Abs(FVector::DotProduct(playerVelocity, lateral));
    context.m_rangedPressure = m_rangedPressure;
    context.m_mobilityPressure = m_mobilityPressure;
    context.m_vulnerabilityPressure = m_vulnerabilityPressure;
    context.m_recentBossDamagePressure = FMath::Clamp(m_recentBossDamagePressure, 0.f, 1.f);
    context.m_recentSuccessfulHitPressure = FMath::Clamp(m_recentSuccessfulHitPressure, 0.f, 1.f);
    context.m_bHasLineOfSight = _bHasLineOfSight;
    context.m_bPlayerAiming = _player->IsAiming();
    context.m_bPlayerReloading = _player->IsReloading();
    context.m_bPlayerHealing = _player->IsHealing();
    context.m_bPlayerSwitchingWeapon = _player->IsSwitchingWeapon();
    context.m_bPlayerUsingRangedWeapon = _player->GetCurrentSlot() != EWeaponSlot::Knife;
    context.m_bPlayerMovingTowardBoss = playerVelocity.SizeSquared2D() > 100.f && directionDot > 0.35f;
    context.m_bPlayerMovingAwayFromBoss = playerVelocity.SizeSquared2D() > 100.f && directionDot < -0.35f;
    context.m_bBossPhaseTwo = _boss->IsPhaseTwo();
    context.m_predictedPlayerLocation = playerLocation + playerVelocity * FMath::Max(0.f, m_predictionSeconds);
    return context;
}

//履歴内の指定されたアクションの出現回数をカウント
int32 UBossUtilityAIComponent::CountRecentAction(EBossTacticalAction _action) const
{
    //履歴内の指定されたアクションの出現回数をカウント
    int32 count = 0;
    for (EBossTacticalAction previous : m_actionHistory)
    {
        if (previous == _action)
        {
            ++count;
        }
    }
    return count;
}

//履歴ペナルティを適用してスコアを調整
float UBossUtilityAIComponent::ApplyHistoryPenalty(EBossTacticalAction _action, float _score) const
{
    //履歴内の指定されたアクションの出現回数を取得
    const int32 repeatCount = CountRecentAction(_action);
    if (repeatCount <= 0) { return _score; }

    //履歴ペナルティを適用してスコアを調整
    return _score * FMath::Pow(FMath::Clamp(1.f - m_repeatPenalty, 0.05f, 1.f), static_cast<float>(repeatCount));
}

//指定されたアクションのスコアを計算
float UBossUtilityAIComponent::ScoreAction(EBossTacticalAction _action, const FBossDecisionContext& _context, const ABossChara* _boss) const
{
    //ボスが存在しない場合はスコアを0にする
    if (!_boss) { return 0.f; }
    if (!_boss->SupportsTacticalAction(_action)) { return 0.f; }
    //遮蔽物越しには攻撃を予約せず、最後に見えた場所への移動を優先する
    const bool attack = _action == EBossTacticalAction::LightCombo || _action == EBossTacticalAction::PowerSlam ||
                        _action == EBossTacticalAction::ChargeRush || _action == EBossTacticalAction::BackStep;
    if (attack && !_context.m_bHasLineOfSight) { return 0.0f; }

    //距離に基づいてスコアを計算
    const float meleeRange = FMath::Max(100.f, _boss->GetBossMeleeRange());
    //攻撃範囲を保持します。
    const float attackRange = FMath::Max(meleeRange, _boss->GetBossAttackRequestRange());
    const float nearRangeScore = 1.f - FMath::Clamp(_context.m_distanceToPlayer / (meleeRange * 1.35f), 0.f, 1.f);
    const float midRangeScore =
        1.f - FMath::Abs(FMath::Clamp((_context.m_distanceToPlayer - meleeRange) / FMath::Max(1.f, attackRange - meleeRange), 0.f, 1.f) - 0.5f) * 2.f;
    const float farRangeScore = FMath::Clamp((_context.m_distanceToPlayer - meleeRange * 1.25f) / FMath::Max(1.f, attackRange - meleeRange), 0.f, 1.f);
    const float lowBossHealth = 1.f - _context.m_bossHealthRatio;
    const float lowPlayerHealth = 1.f - _context.m_playerHealthRatio;
    const float vulnerable =
        //最大を処理します。
        FMath::Max(_context.m_vulnerabilityPressure, (_context.m_bPlayerHealing || _context.m_bPlayerReloading) ? 1.f : 0.f);

    //派生ボスの補正値によって、射程外の攻撃候補が選ばれることを防ぎます。
    if (_action == EBossTacticalAction::LightCombo && _context.m_distanceToPlayer > _boss->GetContactAttackRange()) { return 0.0f; }
    if (_action == EBossTacticalAction::PowerSlam && _context.m_distanceToPlayer > _boss->GetSlamStartRange()) { return 0.0f; }
    if (_action == EBossTacticalAction::ChargeRush &&
        (_context.m_distanceToPlayer <= meleeRange * 1.2f || _context.m_distanceToPlayer > attackRange)) { return 0.0f; }
    if (_action == EBossTacticalAction::BackStep && _context.m_distanceToPlayer > meleeRange * 1.35f) { return 0.0f; }

    //スコアを初期化
    float score = 0.f;

    //アクションに応じてスコアを計算
    switch (_action)
    {
    //アクションがObserveの場合のスコア計算
    case EBossTacticalAction::Observe:
    {
        score = 0.12f + _context.m_mobilityPressure * 0.18f;
        if (!_context.m_bHasLineOfSight)
        {
            score += 0.25f;
        }
        break;
    }
    //アクションがApproachの場合のスコア計算
    case EBossTacticalAction::Approach:
    {
        //遠距離攻撃を持たない中ボスが、射程の外を回り続けないよう接近を優先する
        score = farRangeScore * 0.95f + vulnerable * 0.35f;
        //衝撃波の広い射程をパンチにも使わず、手足が届くまで接近を続ける。
        if (_context.m_distanceToPlayer > _boss->GetContactAttackRange()) { score += 0.45f; }
        if (!_context.m_bHasLineOfSight)
        {
            score += 0.5f;
        }
        break;
    }
    //アクションがCircleLeftまたはCircleRightの場合のスコア計算
    case EBossTacticalAction::CircleLeft:
    case EBossTacticalAction::CircleRight:
    {
        score = midRangeScore * 0.55f + _context.m_rangedPressure * 0.45f + _context.m_mobilityPressure * 0.25f +
                _context.m_recentBossDamagePressure * 0.18f;
        break;
    }
    //アクションがRetreatの場合のスコア計算
    case EBossTacticalAction::Retreat:
    {
        score = nearRangeScore * 0.45f + lowBossHealth * 0.35f + _context.m_recentBossDamagePressure * 0.6f;
        if (_context.m_bPlayerMovingTowardBoss)
        {
            score += 0.25f;
        }
        break;
    }
    //アクションがLightComboの場合のスコア計算
    case EBossTacticalAction::LightCombo:
        if (_context.m_distanceToPlayer <= meleeRange)
        {
            score = nearRangeScore * 0.8f + vulnerable * 0.3f + lowPlayerHealth * 0.2f + _context.m_recentSuccessfulHitPressure * 0.15f;
        }
        break;
    //アクションがPowerSlamの場合のスコア計算
    case EBossTacticalAction::PowerSlam:
        if (_context.m_distanceToPlayer <= meleeRange * 1.15f)
        {
            const float slowPlayer = 1.f - _context.m_mobilityPressure;
            score = nearRangeScore * 0.58f + slowPlayer * 0.32f + vulnerable * 0.52f + (_context.m_bPlayerMovingTowardBoss ? 0.2f : 0.f);
        }
        break;
    //アクションがChargeRushの場合のスコア計算
    case EBossTacticalAction::ChargeRush:
        if (_context.m_distanceToPlayer > meleeRange * 1.2f && _context.m_distanceToPlayer <= attackRange * 1.15f)
        {
            score = farRangeScore * 0.72f + vulnerable * 0.65f + (_context.m_bPlayerMovingAwayFromBoss ? 0.3f : 0.f) +
                    (_context.m_bBossPhaseTwo ? 0.18f : 0.f);
        }
        break;

    //アクションがBackStepの場合のスコア計算
    case EBossTacticalAction::BackStep:
        if (_context.m_distanceToPlayer <= meleeRange * 1.35f)
        {
            score = nearRangeScore * 0.48f + _context.m_recentBossDamagePressure * 0.55f + _context.m_rangedPressure * 0.2f +
                    (_context.m_bPlayerMovingTowardBoss ? 0.28f : 0.f);
        }
        break;

    default: break;
    }

    //ボスのユーティリティスコアを修正
    score = _boss->ModifyUtilityScore(_action, _context, score);

    //履歴ペナルティを適用してスコアを調整
    score = ApplyHistoryPenalty(_action, score);

    //ランダム性を加えてスコアを調整
    score += FMath::FRandRange(0.f, FMath::Max(0.f, m_randomness));
    return FMath::Max(0.f, score);
}

//アクションを選択
EBossTacticalAction UBossUtilityAIComponent::ChooseAction(const FBossDecisionContext& _context, const ABossChara* _boss) const
{
    //アクションのリストを定義
    static const EBossTacticalAction actions[] = {EBossTacticalAction::Observe,     EBossTacticalAction::Approach,   EBossTacticalAction::CircleLeft,
                                                  EBossTacticalAction::CircleRight, EBossTacticalAction::Retreat,    EBossTacticalAction::LightCombo,
                                                  EBossTacticalAction::PowerSlam,   EBossTacticalAction::ChargeRush, EBossTacticalAction::BackStep};

    //最適なアクションを選択
    EBossTacticalAction bestAction = EBossTacticalAction::Observe;

    //最適なスコアを初期化
    float bestScore = -1.f;

    //各アクションのスコアを計算し、最適なアクションを選択
    for (EBossTacticalAction action : actions)
    {
        const float score = ScoreAction(action, _context, _boss);
        if (score > bestScore)
        {
            bestScore = score;
            bestAction = action;
        }
    }
    //最適なアクションを返す
    return bestAction;
}

//アクションを履歴に記録
void UBossUtilityAIComponent::RecordAction(EBossTacticalAction _action)
{
    //アクションを履歴の先頭に挿入
    m_actionHistory.Insert(_action, 0);

    //履歴の長さを制限
    const int32 maxHistory = FMath::Clamp(m_historyLength, 1, 8);

    //履歴の長さが制限を超えた場合、古いアクションを削除
    if (m_actionHistory.Num() > maxHistory)
    {
        m_actionHistory.SetNum(maxHistory);
    }
}

//ボスがダメージを受けたことを通知
void UBossUtilityAIComponent::NotifyBossDamaged(float _damageAmount)
{
    m_recentBossDamagePressure = FMath::Clamp(m_recentBossDamagePressure + FMath::Max(0.f, _damageAmount) / 80.f, 0.f, 1.f);
}

//プレイヤーがヒットしたことを通知
void UBossUtilityAIComponent::NotifyPlayerHit(float _damageAmount)
{
    m_recentSuccessfulHitPressure =
        //値が範囲を超えないように収めます。
        FMath::Clamp(m_recentSuccessfulHitPressure + FMath::Max(0.f, _damageAmount) / 60.f, 0.f, 1.f);
}

//アクションのロック時間を取得
float UBossUtilityAIComponent::GetActionLockDuration(EBossTacticalAction _action) const
{
    //アクションに応じてロック時間を返す
    switch (_action)
    {
        //アクションがObserveの場合のロック時間を返す
    case EBossTacticalAction::Observe:
        return FMath::FRandRange(0.35f, 0.7f);
        //アクションがApproachの場合のロック時間を返す
    case EBossTacticalAction::Approach:
        return FMath::FRandRange(0.55f, 0.95f);
        //アクションがCircleLeftまたはCircleRightの場合のロック時間を返す
    case EBossTacticalAction::CircleLeft:
    case EBossTacticalAction::CircleRight:
        return FMath::FRandRange(0.65f, 1.15f);
        //アクションがLightComboの場合のロック時間を返す
    case EBossTacticalAction::Retreat: return FMath::FRandRange(0.45f, 0.8f);
    default: return 0.25f;
    }
}
