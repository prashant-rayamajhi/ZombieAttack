#include "BossAIController.h"
#include "BossUtilityAIComponent.h"
#include "ZombieAttack/Enemy/BossChara/BossChara.h"
#include "ZombieAttack/Player/PlayerChara.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

//コンストラクタ
ABossAIController::ABossAIController()
    : m_pUtilityAI(nullptr), m_currentAction(EBossTacticalAction::Observe), m_bUseArchetypePreset(true), m_decisionIntervalMin(0.22f),
      m_decisionIntervalMax(0.42f), m_approachStandOffDistance(260.f), m_circleRadius(360.f), m_circleSideOffset(260.f), m_bossRetreatDistance(520.f),
      m_navProjectionExtent(350.f, 350.f, 650.f), m_nextAllowedDecisionTime(0.f)
{
    //コンポーネントを作成
    m_pUtilityAI = CreateDefaultSubobject<UBossUtilityAIComponent>(TEXT("BossUtilityAI"));
}

//ポーンを所持したときの処理
void ABossAIController::OnPossess(APawn* _inPawn)
{
    //親クラスの処理を呼び出す
    Super::OnPossess(_inPawn);

    //ボスの戦術行動を初期化
    m_currentAction = EBossTacticalAction::Observe;
    m_nextAllowedDecisionTime = 0.f;

    //ボスのアーキタイプに応じてパラメータを設定
    if (m_bUseArchetypePreset)
    {
        //ボスキャラクターにキャスト
        if (ABossChara* boss = Cast<ABossChara>(_inPawn))
        {
            //ボスのAIアーキタイプを取得
            const EBossAIArchetype bossAIArchetype = boss->GetBossAIArchetype();

            //アーキタイプに応じてパラメータを設定
            switch (bossAIArchetype)
            {
            //中ボスのアーキタイプに応じてパラメータを設定
            case EBossAIArchetype::MidBoss:
            {
                m_decisionIntervalMin = 0.28f;
                m_decisionIntervalMax = 0.48f;
                m_approachStandOffDistance = 260.f;
                m_circleRadius = 360.f;
                m_circleSideOffset = 220.f;
                m_bossRetreatDistance = 480.f;
                break;
            }
            //最終ボスのアーキタイプに応じてパラメータを設定
            case EBossAIArchetype::FinalBoss:
            {
                m_decisionIntervalMin = 0.18f;
                m_decisionIntervalMax = 0.35f;
                m_approachStandOffDistance = 280.f;
                m_circleRadius = 420.f;
                m_circleSideOffset = 300.f;
                m_bossRetreatDistance = 560.f;
                break;
            }
            default: break;
            }

            //ユーティリティAIコンポーネントにアーキタイプを設定
            if (m_pUtilityAI)
            {
                m_pUtilityAI->ConfigureForArchetype(bossAIArchetype);
            }
        }
    }
    //次の意思決定をスケジュール
    ScheduleNextDecision(FMath::FRandRange(0.25f, 0.6f));
}

//ポーンの所持を解除したときの処理
void ABossAIController::OnUnPossess()
{
    //タイマーをクリア
    GetWorldTimerManager().ClearTimer(m_decisionTimer);

    //親クラスの処理を呼び出す
    Super::OnUnPossess();
}

//次の意思決定をスケジュールする
void ABossAIController::ScheduleNextDecision(float _overrideDelay)
{
    //ポーンが存在しない場合は処理を終了
    if (!GetPawn()) { return; }

    //既存のタイマーをクリア
    GetWorldTimerManager().ClearTimer(m_decisionTimer);
    const float Delay =
        _overrideDelay >= 0.f ? _overrideDelay : FMath::FRandRange(m_decisionIntervalMin, FMath::Max(m_decisionIntervalMin, m_decisionIntervalMax));

    //タイマーを設定してEvaluateDecisionを呼び出す
    GetWorldTimerManager().SetTimer(m_decisionTimer, this, &ABossAIController::EvaluateDecision, FMath::Max(0.1f, Delay), false);
}

//意思決定を評価する
void ABossAIController::EvaluateDecision()
{
    //ボスキャラクターとプレイヤーキャラクターを取得
    ABossChara* boss = Cast<ABossChara>(GetPawn());
    //プレイヤーを保持します。
    APlayerChara* player = Cast<APlayerChara>(GetSensedActor());

    //ボスが存在しない、死亡している、または状態が死亡の場合は処理を終了
    if (!boss || boss->IsDead() || GetCurrentState() == EEnemyAIState::Dead) { return; }

    //プレイヤーが存在しない、または死亡している場合は次の意思決定をスケジュールして終了
    if (!player || player->IsDead())
    {
        ScheduleNextDecision(0.45f);
        return;
    }

    //ボスが戦術行動を実行できない、または現在の状態が攻撃中の場合は次の意思決定をスケジュールして終了
    if (!boss->CanPerformTacticalAction() || GetCurrentState() == EEnemyAIState::Attack)
    {
        ScheduleNextDecision(0.2f);
        return;
    }

    //現在の時間を取得し、次の意思決定が許可されているかを確認
    const float currentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    //「currentTime < m_nextAllowedDecisionTime」が成立するとき、ScheduleNextDecisionを呼び出します。
    if (currentTime < m_nextAllowedDecisionTime)
    {
        ScheduleNextDecision(m_nextAllowedDecisionTime - currentTime);
        return;
    }

    //プレイヤーへの視線があるかどうかを確認
    const bool bHasLineOfSight = LineOfSightTo(player);

    //ユーティリティAIコンポーネントを使用して意思決定コンテキストを構築
    const FBossDecisionContext context = m_pUtilityAI ? m_pUtilityAI->BuildDecisionContext(boss, player, bHasLineOfSight) : FBossDecisionContext();

    //ユーティリティAIコンポーネントを使用して戦術行動を選択
    const EBossTacticalAction action = m_pUtilityAI ? m_pUtilityAI->ChooseAction(context, boss) : EBossTacticalAction::Approach;

    //デバッグ用のログを出力

    //選択された戦術行動を実行
    if (ExecuteAction(action, boss, player, context))
    {
        //戦術行動が成功した場合、現在の戦術行動を更新し、ユーティリティAIコンポーネントに記録
        m_currentAction = action;
        //「m_pUtilityAI」が成立するとき、RecordActionを呼び出します。
        if (m_pUtilityAI)
        {
            m_pUtilityAI->RecordAction(action);
            m_nextAllowedDecisionTime = currentTime + m_pUtilityAI->GetActionLockDuration(action);
        }
    }
    else
    {
        //戦術行動が失敗した場合、デフォルトの戦術行動（接近）を実行
        m_currentAction = EBossTacticalAction::Approach;
        ExecuteAction(m_currentAction, boss, player, context);
        m_nextAllowedDecisionTime = currentTime + 0.3f;
    }
    //次の意思決定をスケジュール
    ScheduleNextDecision();
}

//戦術的な位置に移動する
bool ABossAIController::MoveToTacticalLocation(const FVector& _desiredLocation, float _acceptanceRadius)
{
    //ナビゲーションシステムを取得
    UWorld* world = GetWorld();
    //現在のを返します。
    UNavigationSystemV1* NavSystem = world ? UNavigationSystemV1::GetCurrent(world) : nullptr;
    //「!NavSystem」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (!NavSystem) { return false; }

    //ナビゲーションシステムを使用して、希望する位置をナビゲーションメッシュ上に投影
    FNavLocation projected;
    //「!NavSystem->ProjectPointToNavigation(_desiredLocation, projected, m_navProjectionExtent)」が成立するとき、この関数を終了します。
    if (!NavSystem->ProjectPointToNavigation(_desiredLocation, projected, m_navProjectionExtent)) { return false; }

    //移動要求を送信
    const EPathFollowingRequestResult::Type Result = MoveToLocation(projected.Location, FMath::Max(5.f, _acceptanceRadius), true, true, false, true);
    return Result != EPathFollowingRequestResult::Failed;
}

//戦術行動を実行する
bool ABossAIController::ExecuteAction(EBossTacticalAction _action, ABossChara* _boss, APlayerChara* _player, const FBossDecisionContext& _context)
{
    //ボスまたはプレイヤーが存在しない場合は処理を終了
    if (!_boss || !_player) { return false; }

    //プレイヤーに視線を設定
    SetFocus(_player, EAIFocusPriority::Gameplay);

    //ボスとプレイヤーの位置を取得し、プレイヤーからボスへの方向を計算
    const FVector bossLocation = _boss->GetActorLocation();
    //プレイヤー位置を保持します。
    const FVector playerLocation = _player->GetActorLocation();
    //fromPlayerToBossは、(bossLocation - playerLocation).GetSafeNormal2D()から求めた空間情報を位置または向きの計算に使います。
    FVector fromPlayerToBoss = (bossLocation - playerLocation).GetSafeNormal2D();

    //プレイヤーからボスへの方向がゼロベクトルの場合、ボスの前方ベクトルを使用
    if (fromPlayerToBoss.IsNearlyZero())
    {
        fromPlayerToBoss = _boss->GetActorForwardVector();
    }

    //プレイヤーからボスへの方向の垂直ベクトルを計算
    const FVector Tangent(-fromPlayerToBoss.Y, fromPlayerToBoss.X, 0.f);

    //戦術行動に応じて処理を分岐
    switch (_action)
    {
    //戦術行動が観察の場合、移動を停止
    case EBossTacticalAction::Observe:
    {
        StopMovement();
        //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
        return true;
    }
    //戦術行動が接近の場合、プレイヤーに近づく
    case EBossTacticalAction::Approach:
    {
        //desiredは、_context.PredictedPlayerLocation + fromPlayerToBoss * m_approachStandOf…から求めた空間情報を位置または向きの計算に使います。
        const FVector desired = _context.PredictedPlayerLocation + fromPlayerToBoss * m_approachStandOffDistance;
        return MoveToTacticalLocation(desired, 75.f);
    }
    //戦術行動が回避の場合、プレイヤーから離れる
    case EBossTacticalAction::CircleLeft:
    case EBossTacticalAction::CircleRight:
    {
        //sideは、_action == EBossTacticalAction::CircleRight ? 1.f : -1.fから算出した数値を後続の判定または計算に使います。
        const float side = _action == EBossTacticalAction::CircleRight ? 1.f : -1.f;
        //desiredは、playerLocation + fromPlayerToBoss * m_circleRadius + Tangent * side * m…から求めた空間情報を位置または向きの計算に使います。
        const FVector desired = playerLocation + fromPlayerToBoss * m_circleRadius + Tangent * side * m_circleSideOffset;
        return MoveToTacticalLocation(desired, 60.f);
    }
    //後退行動を実行
    case EBossTacticalAction::Retreat:
    {
        //desiredは、bossLocation + fromPlayerToBoss * m_bossRetreatDistance + Tangent * FMa…から求めた空間情報を位置または向きの計算に使います。
        const FVector desired = bossLocation + fromPlayerToBoss * m_bossRetreatDistance + Tangent * FMath::FRandRange(-160.f, 160.f);
        return MoveToTacticalLocation(desired, 65.f);
    }
    //戦術行動がライトコンボの場合、特定の攻撃を要求
    case EBossTacticalAction::LightCombo:
    {
        return _boss->RequestSpecificAttack(EBossAttackPattern::LightCombo, _context.PredictedPlayerLocation);
    }
    //戦術行動がヘビーコンボの場合、特定の攻撃を要求
    case EBossTacticalAction::PowerSlam:
    {
        return _boss->RequestSpecificAttack(EBossAttackPattern::PowerSlam, _context.PredictedPlayerLocation);
    }
    //戦術行動がチャージラッシュの場合、特定の攻撃を要求
    case EBossTacticalAction::ChargeRush:
    {
        return _boss->RequestSpecificAttack(EBossAttackPattern::ChargeRush, _context.PredictedPlayerLocation);
    }
    //戦術行動がバックステップの場合、特定の攻撃を要求
    case EBossTacticalAction::BackStep:
    {
        return _boss->RequestSpecificAttack(EBossAttackPattern::BackStep, _context.PredictedPlayerLocation);
    }
    default: return false;
    }
}

//ボスがダメージを受けたことを通知する
void ABossAIController::NotifyBossDamaged(float _damageAmount)
{
    //ユーティリティAIコンポーネントにダメージを通知
    if (m_pUtilityAI)
    {
        m_pUtilityAI->NotifyBossDamaged(_damageAmount);
    }

    //次の意思決定をすぐに行えるようにする
    m_nextAllowedDecisionTime = 0.f;
    ScheduleNextDecision(0.08f);
}

//プレイヤーがダメージを受けたことを通知する
void ABossAIController::NotifyPlayerHit(float _damageAmount)
{
    //ユーティリティAIコンポーネントにプレイヤーへのヒットを通知
    if (m_pUtilityAI)
    {
        m_pUtilityAI->NotifyPlayerHit(_damageAmount);
    }
}

//攻撃が終了したことを通知する
void ABossAIController::NotifyAttackFinished()
{
    //親クラスの処理を呼び出す
    Super::NotifyAttackFinished();

    //次の意思決定をすぐに行えるようにする
    m_nextAllowedDecisionTime = 0.f;
    ScheduleNextDecision(0.12f);
}

//すべてのAIロジックを停止する
void ABossAIController::StopAllLogic()
{
    //タイマーをクリアして意思決定を停止
    GetWorldTimerManager().ClearTimer(m_decisionTimer);
    Super::StopAllLogic();
}
