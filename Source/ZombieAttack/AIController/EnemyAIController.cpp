#include "EnemyAIController.h"
#include "../Enemy/EnemyChara.h"
#include "../Player/PlayerChara.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"

namespace
{
//g_bSharingEnemyAlertは、falseの成立可否を後続の分岐で判定するために使います。
bool g_bSharingEnemyAlert = false;
constexpr float SquadAlertRadius = 1800.0f;
//名前空間を閉じます。
//名前空間を閉じます。
}

//コンストラクタ
AEnemyAIController::AEnemyAIController()
    : m_pPerception(nullptr), m_pSightConfig(nullptr), m_pHearingConfig(nullptr), m_pSensedTarget(nullptr), m_currentState(EEnemyAIState::Patrol),
      m_currentManeuver(EEnemyCombatManeuver::None), m_patrolRadius(1200.f), m_minPatrolMoveDistance(250.f), m_patrolWaitMin(1.0f),
      m_patrolWaitMax(3.5f), m_searchLookDuration(4.f), m_chaseAcceptanceRadius(130.f), m_tacticalUpdateInterval(0.38f),
      m_targetPredictionSeconds(0.35f), m_flankDistance(260.f), m_retreatDistance(220.f), m_flankSwitchMin(2.2f), m_flankSwitchMax(5.5f),
      m_tacticalNavProjectionExtent(300.f, 300.f, 600.f), m_lastKnownLocation(FVector::ZeroVector), m_flankSign(1.f), m_bAlertReactionActive(false)
{
    //AI Perceptionコンポーネントを作成し、SightとHearingの設定を行う
    m_pPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
    SetPerceptionComponent(*m_pPerception);

    //視野角の設定
    m_pSightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    m_pSightConfig->SightRadius = 2000.f;
    m_pSightConfig->LoseSightRadius = 2300.f;
    m_pSightConfig->PeripheralVisionAngleDegrees = 80.f;
    m_pSightConfig->SetMaxAge(3.f);
    m_pSightConfig->DetectionByAffiliation.bDetectEnemies = true;
    m_pSightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    m_pSightConfig->DetectionByAffiliation.bDetectNeutrals = true;

    //聴覚の設定
    m_pHearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    m_pHearingConfig->HearingRange = 1800.f;
    m_pHearingConfig->SetMaxAge(5.f);
    m_pHearingConfig->DetectionByAffiliation.bDetectEnemies = true;
    m_pHearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
    m_pHearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

    //PerceptionコンポーネントにSenseの設定を追加し、DominantSenseをSightに設定
    m_pPerception->ConfigureSense(*m_pSightConfig);
    m_pPerception->ConfigureSense(*m_pHearingConfig);
    m_pPerception->SetDominantSense(m_pSightConfig->GetSenseImplementation());
    m_pPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetPerceptionUpdated);
}

//ポーンを制御する際の初期化処理
void AEnemyAIController::OnPossess(APawn* _inPawn)
{
    //親クラスのOnPossessを呼び出す
    Super::OnPossess(_inPawn);

    //フランクの方向をランダムに設定
    m_flankSign = FMath::RandBool() ? 1.f : -1.f;

    //ポーンが有効である場合、パトロール状態に設定し、次のパトロールをスケジュールする
    SetState(EEnemyAIState::Patrol);
    ScheduleNextPatrol(FMath::FRandRange(0.15f, 1.0f));
}

//ポーンの制御を解除する際の処理
void AEnemyAIController::OnUnPossess()
{
    //タイマーをクリアし、ターゲットと戦術的な動作をリセット
    GetWorldTimerManager().ClearAllTimersForObject(this);
    m_pSensedTarget = nullptr;
    m_currentManeuver = EEnemyCombatManeuver::None;
    //親クラスのOnUnPossessを呼び出す
    Super::OnUnPossess();
}

//AIの状態を設定する
void AEnemyAIController::SetState(EEnemyAIState NewState) { m_currentState = NewState; }

//パトロールを開始する
void AEnemyAIController::StartRandomPatrol()
{
    //敵が死亡している場合はパトロールを開始しない
    if (m_currentState == EEnemyAIState::Dead) { return; }

    //パトロールを停止し、戦術的な追跡の更新を停止する
    StopTacticalChaseUpdates();

    //ターゲットをリセットし、戦術的な動作をリセットする
    m_pSensedTarget = nullptr;
    m_currentManeuver = EEnemyCombatManeuver::None;
    SetState(EEnemyAIState::Patrol);
    ClearFocus(EAIFocusPriority::Gameplay);
    ScheduleNextPatrol(0.f);
}

//パトロールを停止する
void AEnemyAIController::StopPatrol() { GetWorldTimerManager().ClearTimer(m_patrolTimer); }

//次のパトロールをスケジュールする
void AEnemyAIController::ScheduleNextPatrol(float _overrideDelay)
{
    //パトロール状態でない場合やポーンが存在しない場合は処理を終了する
    if (m_currentState != EEnemyAIState::Patrol || !GetPawn()) { return; }

    //既存のパトロールタイマーをクリアし、次のパトロールの遅延時間を決定する
    GetWorldTimerManager().ClearTimer(m_patrolTimer);
    //Delayは、_overrideDelay >= 0.f ? _overrideDelay : FMath::FRandRange(m_patrolWait…から算出した数値を後続の判定または計算に使います。
    const float Delay = _overrideDelay >= 0.f ? _overrideDelay : FMath::FRandRange(m_patrolWaitMin, FMath::Max(m_patrolWaitMin, m_patrolWaitMax));

    //次のパトロールポイントへの移動をスケジュールする
    GetWorldTimerManager().SetTimer(m_patrolTimer, this, &AEnemyAIController::MoveToNextPatrolPoint, Delay, false);
}

//次のパトロールポイントに移動する
void AEnemyAIController::MoveToNextPatrolPoint()
{
    //パトロール状態でない場合は処理を終了する
    if (m_currentState != EEnemyAIState::Patrol) { return; }

    //制御されているポーンとワールドを取得する
    APawn* pawn = GetPawn();
    //ワールドを返します。
    UWorld* world = GetWorld();

    //制御されているポーンまたはワールドが存在しない場合、次のパトロールをスケジュールして処理を終了する
    if (!pawn || !world)
    {
        ScheduleNextPatrol(0.5f);
        return;
    }

    //ナビゲーションシステムを取得する
    UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(world);
    //「!navSystem」が成立するとき、ScheduleNextPatrolを呼び出します。
    if (!navSystem)
    {
        ScheduleNextPatrol(0.5f);
        return;
    }

    //ナビゲーションシステムを使用して、指定された半径内で到達可能なランダムなポイントを取得する
    FNavLocation candidate;
    //探索対象を有効な範囲内で発見できたかを示します。
    bool bFound = false;
    //「int32 attempt = 0; attempt < 12; ++attempt」の範囲を走査し、続けて「navSystem->GetRandomReachablePointInRadius(pawn->GetActorLocation(), …」を判定します。
    for (int32 attempt = 0; attempt < 12; ++attempt)
    {
        //ナビゲーションシステムを使用して、指定された半径内で到達可能なランダムなポイントを取得する
        if (navSystem->GetRandomReachablePointInRadius(pawn->GetActorLocation(), m_patrolRadius, candidate) &&
            FVector::Dist2D(candidate.Location, pawn->GetActorLocation()) >= m_minPatrolMoveDistance)
        {
            bFound = true;
            break;
        }
    }

    //到達可能なポイントが見つからなかった場合、次のパトロールをスケジュールして処理を終了する
    if (!bFound)
    {
        ScheduleNextPatrol(0.75f);
        return;
    }

    //見つかったポイントに移動する
    const EPathFollowingRequestResult::Type result = MoveToLocation(candidate.Location, 75.f, true, true, false, true);

    //移動が成功しなかった場合、次のパトロールをスケジュールする
    if (result != EPathFollowingRequestResult::RequestSuccessful)
    {
        ScheduleNextPatrol(0.5f);
    }
}

//指定された位置での探索を開始する
void AEnemyAIController::StartSearch(const FVector& _searchLocation)
{
    //敵が死亡している場合は探索を開始しない
    if (m_currentState == EEnemyAIState::Dead) { return; }

    //パトロールを停止し、戦術的な追跡の更新を停止する
    StopPatrol();
    StopTacticalChaseUpdates();
    GetWorldTimerManager().ClearTimer(m_searchTimer);
    StopMovement();

    //探索位置を記録し、探索状態に設定する
    m_lastKnownLocation = _searchLocation;
    m_currentManeuver = EEnemyCombatManeuver::None;
    SetState(EEnemyAIState::Search);

    //探索位置に移動する
    const EPathFollowingRequestResult::Type result = MoveToLocation(_searchLocation, 90.f, true, true, false, true);

    //移動が成功しなかった場合、探索を終了してパトロールを開始する
    if (result != EPathFollowingRequestResult::RequestSuccessful)
    {
        GetWorldTimerManager().SetTimer(m_searchTimer, this, &AEnemyAIController::FinishSearch, 0.5f, false);
    }
}

//探索を終了し、パトロールを開始する
void AEnemyAIController::FinishSearch()
{
    //探索タイマーをクリアし、探索状態である場合にパトロールを開始する
    if (m_currentState == EEnemyAIState::Search)
    {
        StartRandomPatrol();
    }
}

//指定されたターゲットを追跡する
void AEnemyAIController::StartChase(AActor* _target)
{
    //ターゲットが有効でない場合や敵が死亡している場合は追跡を開始しない
    if (!IsValid(_target) || m_currentState == EEnemyAIState::Dead) { return; }

    //今回の知覚更新がプレイヤーを初めて発見した瞬間かを示します。
    const bool bFirstDetection = m_currentState == EEnemyAIState::Patrol || m_currentState == EEnemyAIState::Search;

    //パトロールを停止し、探索タイマーをクリアする
    StopPatrol();
    GetWorldTimerManager().ClearTimer(m_searchTimer);

    //ターゲットを記録し、最後に知られている位置を更新する
    m_pSensedTarget = _target;
    m_lastKnownLocation = _target->GetActorLocation();
    SetState(EEnemyAIState::Chase);
    SetFocus(_target, EAIFocusPriority::Gameplay);

    //Combat状態へ入った直後はAnimBPのScreamが再生されます。
    //その間だけPathFollowingを止め、全身アニメーション中の足滑りを防ぎます。
    if (bFirstDetection)
    {
        m_bAlertReactionActive = true;
        StopTacticalChaseUpdates();
        StopMovement();
        //「ACharacter* character = Cast<ACharacter>(GetPawn())」が成立するとき、GetCharacterMovementを呼び出します。
        if (ACharacter* character = Cast<ACharacter>(GetPawn()))
        {
            character->GetCharacterMovement()->StopMovementImmediately();
        }
        GetWorldTimerManager().ClearTimer(m_alertReactionTimer);
        GetWorldTimerManager().SetTimer(m_alertReactionTimer, this, &AEnemyAIController::ResumeChaseAfterAlert, 1.05f, false);
    }

    //戦術的な追跡を使用するかどうかを判断し、適切な追跡方法を開始する
    if (!m_bAlertReactionActive && ShouldUseTacticalChase())
    {
        StartTacticalChaseUpdates();
        UpdateTacticalChase();
    }
    else if (!m_bAlertReactionActive)
    {
        MoveToActor(_target, m_chaseAcceptanceRadius, true, true, true, nullptr, true);
    }

    //周囲のゾンビを集団で反応させ、通知の中継が再帰的に繰り返されることを防ぎます。
    if (!g_bSharingEnemyAlert && GetWorld() && GetPawn())
    {
        //AlertGuardは、g_bSharingEnemyAlert, true)から構築した結果を後続の処理へ渡すために使います。
        TGuardValue<bool> AlertGuard(g_bSharingEnemyAlert, true);
        //「TActorIterator<AEnemyChara> Iterator(GetWorld()); Iterator; ++Iterator」で列挙される各要素へ、ループ本体の判定と更新を適用します。
        for (TActorIterator<AEnemyChara> Iterator(GetWorld()); Iterator; ++Iterator)
        {
            //Allyは、*Iteratorから取得した参照を後続の呼び出しで使います。
            AEnemyChara* Ally = *Iterator;
            //「!IsValid(Ally) || Ally == GetPawn(」が成立するとき、DistSquared2Dを呼び出します。
            if (!IsValid(Ally) || Ally == GetPawn() ||
                FVector::DistSquared2D(Ally->GetActorLocation(), GetPawn()->GetActorLocation()) > FMath::Square(SquadAlertRadius))
            {
                continue;
            }

            //「AEnemyAIController* AllyController」が成立するとき、GetControllerを呼び出します。
            if (AEnemyAIController* AllyController = Cast<AEnemyAIController>(Ally->GetController()))
            {
                //現在の状態を返します。
                const EEnemyAIState AllyState = AllyController->GetCurrentState();
                //「AllyState == EEnemyAIState::Patrol || AllyState == EEnemyAIState::Search」が成立するとき、StartChaseを呼び出します。
                if (AllyState == EEnemyAIState::Patrol || AllyState == EEnemyAIState::Search)
                {
                    AllyController->StartChase(_target);
                }
            }
        }
    }
}

//ChaseAfterAlertを中断位置から再開します。
void AEnemyAIController::ResumeChaseAfterAlert()
{
    m_bAlertReactionActive = false;
    //「m_currentState != EEnemyAIState::Chase || !IsValid(m_pSensedTarget) || !GetPawn()」が成立するとき、続けて「ShouldUseTacticalChase()」を判定します。
    if (m_currentState != EEnemyAIState::Chase || !IsValid(m_pSensedTarget) || !GetPawn()) { return; }

    //「ShouldUseTacticalChase()」が成立するとき、StartTacticalChaseUpdatesを呼び出します。
    if (ShouldUseTacticalChase())
    {
        StartTacticalChaseUpdates();
        UpdateTacticalChase();
    }
    else
    {
        MoveToActor(m_pSensedTarget.Get(), m_chaseAcceptanceRadius, true, true, true, nullptr, true);
    }
}

//追跡を停止し、パトロールを開始する
void AEnemyAIController::StopChase()
{
    //敵が死亡している場合は追跡を停止しない
    if (m_currentState == EEnemyAIState::Dead) { return; }

    //パトロールを開始する前に、戦術的な追跡の更新を停止する
    StopTacticalChaseUpdates();
    GetWorldTimerManager().ClearTimer(m_alertReactionTimer);
    m_bAlertReactionActive = false;
    m_pSensedTarget = nullptr;
    m_currentManeuver = EEnemyCombatManeuver::None;
    ClearFocus(EAIFocusPriority::Gameplay);
    StopMovement();
    //追跡を停止した後、パトロールを開始する
    StartRandomPatrol();
}

//戦術的な追跡の更新を開始する
void AEnemyAIController::StartTacticalChaseUpdates()
{
    //戦術的な追跡を使用するかどうかを判断し、使用しない場合は処理を終了する
    if (!ShouldUseTacticalChase()) { return; }

    //既存の戦術的な追跡タイマーをクリアし、新しいタイマーを設定する
    GetWorldTimerManager().ClearTimer(m_tacticalChaseTimer);
    GetWorldTimerManager().SetTimer(m_tacticalChaseTimer, this, &AEnemyAIController::UpdateTacticalChase, FMath::Max(0.1f, m_tacticalUpdateInterval),
                                    true);

    //フランクの方向を更新するためのタイマーを設定する
    RefreshFlankDirection();
}

//戦術的な追跡の更新を停止する
void AEnemyAIController::StopTacticalChaseUpdates()
{
    GetWorldTimerManager().ClearTimer(m_tacticalChaseTimer);
    GetWorldTimerManager().ClearTimer(m_flankSwitchTimer);
}

//フランクの方向を更新する
void AEnemyAIController::RefreshFlankDirection()
{
    //フランクの方向を反転させ、フランクの方向を更新するためのタイマーを設定する
    m_flankSign = -m_flankSign;
    GetWorldTimerManager().ClearTimer(m_flankSwitchTimer);
    GetWorldTimerManager().SetTimer(m_flankSwitchTimer, this, &AEnemyAIController::RefreshFlankDirection,
                                    FMath::FRandRange(m_flankSwitchMin, FMath::Max(m_flankSwitchMin, m_flankSwitchMax)), false);
}

//指定された位置をナビゲーションメッシュ上に投影する
bool AEnemyAIController::ProjectToNavigation(const FVector& _desiredLocation, FVector& _outProjectedLocation) const
{
    //ワールドとナビゲーションシステムを取得する
    UWorld* world = GetWorld();
    //現在のを返します。
    UNavigationSystemV1* navSystem = world ? UNavigationSystemV1::GetCurrent(world) : nullptr;
    //「!navSystem」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (!navSystem) { return false; }

    //ナビゲーションシステムを使用して、指定された位置をナビゲーションメッシュ上に投影する
    FNavLocation projected;
    //「!navSystem->ProjectPointToNavigation(_desiredLocation, projected, m_tacticalNavProjection…」が成立するとき、_outProjectedLocationを更新します。
    if (!navSystem->ProjectPointToNavigation(_desiredLocation, projected, m_tacticalNavProjectionExtent)) { return false; }

    //投影された位置を出力パラメータに設定する
    _outProjectedLocation = projected.Location;
    //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
    return true;
}

//戦術的な追跡を更新する
void AEnemyAIController::UpdateTacticalChase()
{
    //戦術的な追跡の更新を行う条件を確認する
    if (m_currentState != EEnemyAIState::Chase || !IsValid(m_pSensedTarget) || !GetPawn()) { return; }

    //ターゲットと敵キャラクターを取得する
    AActor* target = m_pSensedTarget.Get();
    //敵を保持します。
    AEnemyChara* enemy = Cast<AEnemyChara>(GetPawn());
    //「!target || !enemy」が成立するとき、GetActorLocationを呼び出します。
    if (!target || !enemy) { return; }

    //ターゲットの位置、速度、および予測位置を計算する
    const FVector pawnLocation = enemy->GetActorLocation();
    //対象位置を保持します。
    const FVector targetLocation = target->GetActorLocation();
    //速度を返します。
    const FVector targetVelocity = target->GetVelocity();
    //predictedTargetは、targetLocation + targetVelocity * FMath::Max(0.f, m_targetPredictionSec…から求めた空間情報を位置または向きの計算に使います。
    const FVector predictedTarget = targetLocation + targetVelocity * FMath::Max(0.f, m_targetPredictionSeconds);

    //敵の望ましい戦闘距離と現在の距離を計算する
    const float desiredRange = FMath::Max(100.f, enemy->GetDesiredCombatRange());
    //距離を保持します。
    const float distance = FVector::Dist2D(pawnLocation, targetLocation);
    //awayFromTargetは、(pawnLocation - targetLocation).GetSafeNormal2D()から求めた空間情報を位置または向きの計算に使います。
    const FVector awayFromTarget = (pawnLocation - targetLocation).GetSafeNormal2D();
    //tangentは、tangentの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    const FVector tangent(-awayFromTarget.Y, awayFromTarget.X, 0.f);

    //望ましい位置と受け入れ半径を初期化する
    FVector desiredLocation = predictedTarget;
    //acceptanceRadiusは、m_chaseAcceptanceRadiusから算出した数値を後続の判定または計算に使います。
    float acceptanceRadius = m_chaseAcceptanceRadius;

    //距離に応じて戦術的な動作を決定する
    if (distance > desiredRange * 1.75f || awayFromTarget.IsNearlyZero())
    {
        //古い位置を追うのではなく、プレイヤーの移動先を予測して先回りする
        m_currentManeuver = EEnemyCombatManeuver::Intercept;
        //slotAngleは、FMath::DegreesToRadians(FMath::Fmod(static_cast<float>(GetUniqueID()) *…から算出した数値を後続の判定または計算に使います。
        const float slotAngle = FMath::DegreesToRadians(FMath::Fmod(static_cast<float>(GetUniqueID()) * 137.5f, 360.f));
        //slotOffsetは、slotOffsetの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
        const FVector slotOffset(FMath::Cos(slotAngle) * desiredRange * 0.35f, FMath::Sin(slotAngle) * desiredRange * 0.35f, 0.f);
        desiredLocation = predictedTarget + slotOffset;
        acceptanceRadius = desiredRange * 0.65f;
    }
    else if (distance < desiredRange * 0.65f)
    {
        //攻撃モーション用の間合いを作り、敵同士が同じ場所に固まるのを防ぎます。
        m_currentManeuver = EEnemyCombatManeuver::Retreat;
        desiredLocation = pawnLocation + awayFromTarget * m_retreatDistance;
        acceptanceRadius = 45.f;
    }
    else
    {
        //プレイヤーの周囲を回り込みます。敵ごとに方向転換タイミングをずらします。
        m_currentManeuver = m_flankSign > 0.f ? EEnemyCombatManeuver::FlankRight : EEnemyCombatManeuver::FlankLeft;
        //tangentWeightは、FMath::Clamp(m_flankDistance / FMath::Max(1.f, desiredRange), 0.25f, 1.…から算出した数値を後続の判定または計算に使います。
        const float tangentWeight = FMath::Clamp(m_flankDistance / FMath::Max(1.f, desiredRange), 0.25f, 1.25f);
        //orbitDirectionは、(awayFromTarget + tangent * m_flankSign * tangentWeight).GetSafeNormal2…から求めた空間情報を位置または向きの計算に使います。
        const FVector orbitDirection = (awayFromTarget + tangent * m_flankSign * tangentWeight).GetSafeNormal2D();
        desiredLocation = targetLocation + orbitDirection * desiredRange;
        acceptanceRadius = 55.f;
    }

    //ナビゲーションメッシュ上に望ましい位置を投影し、移動を開始する
    FVector projectedLocation;
    //「ProjectToNavigation(desiredLocation, projectedLocation)」が成立するとき、MoveToLocationを呼び出します。
    if (ProjectToNavigation(desiredLocation, projectedLocation))
    {
        MoveToLocation(projectedLocation, acceptanceRadius, true, true, false, true);
    }
    else
    {
        MoveToActor(target, m_chaseAcceptanceRadius, true, true, true, nullptr, true);
    }
}

//攻撃が開始されたことを通知する
void AEnemyAIController::NotifyAttackStarted()
{
    //敵が死亡している場合は攻撃を開始しない
    if (m_currentState == EEnemyAIState::Dead) { return; }

    //戦術的な追跡の更新を停止し、現在の戦術的な動作をリセットする
    StopTacticalChaseUpdates();
    GetWorldTimerManager().ClearTimer(m_alertReactionTimer);
    m_bAlertReactionActive = false;
    m_currentManeuver = EEnemyCombatManeuver::None;
    SetState(EEnemyAIState::Attack);
    StopMovement();
    //「ACharacter* character = Cast<ACharacter>(GetPawn())」が成立するとき、GetCharacterMovementを呼び出します。
    if (ACharacter* character = Cast<ACharacter>(GetPawn()))
    {
        character->GetCharacterMovement()->StopMovementImmediately();
    }
}

//攻撃が終了したことを通知する
void AEnemyAIController::NotifyAttackFinished()
{
    //敵が死亡している場合は攻撃終了後の処理を行わない
    if (m_currentState == EEnemyAIState::Dead) { return; }

    //戦術的な追跡の更新を停止し、現在の戦術的な動作をリセットする
    if (IsValid(m_pSensedTarget))
    {
        StartChase(m_pSensedTarget.Get());
    }
    else
    {
        StartRandomPatrol();
    }
}

//すべてのAIロジックを停止する
void AEnemyAIController::StopAllLogic()
{
    //敵が死亡している場合はすべてのロジックを停止する
    GetWorldTimerManager().ClearAllTimersForObject(this);
    m_bAlertReactionActive = false;
    StopMovement();
    ClearFocus(EAIFocusPriority::Gameplay);
    m_pSensedTarget = nullptr;
    m_currentManeuver = EEnemyCombatManeuver::None;
    SetState(EEnemyAIState::Dead);
    //「m_pPerception」が成立するとき、SetActiveを呼び出します。
    if (m_pPerception)
    {
        m_pPerception->SetActive(false);
    }
}

//移動が完了した際の処理
void AEnemyAIController::OnMoveCompleted(FAIRequestID _requestID, const FPathFollowingResult& _result)
{
    //親クラスのOnMoveCompletedを呼び出す
    Super::OnMoveCompleted(_requestID, _result);

    //現在の状態に応じて次の行動をスケジュールする
    switch (m_currentState)
    {
    case EEnemyAIState::Patrol: ScheduleNextPatrol(); break;
    case EEnemyAIState::Search:
        GetWorldTimerManager().SetTimer(m_searchTimer, this, &AEnemyAIController::FinishSearch, m_searchLookDuration, false);
        break;
    default: break;
    }
}

//ターゲットの知覚が更新された際の処理
void AEnemyAIController::OnTargetPerceptionUpdated(AActor* _actor, FAIStimulus _stimulus)
{
    //ターゲットがプレイヤーキャラクターでない場合、死亡している場合、または敵が死亡している場合は処理を終了する
    APlayerChara* player = Cast<APlayerChara>(_actor);
    //死亡状態かどうかを返します。
    if (!player || player->IsDead() || m_currentState == EEnemyAIState::Dead) { return; }

    //知覚の種類を判定する
    const bool bSight = _stimulus.Type == UAISense::GetSenseID<UAISenseConfig_Sight>();
    //今回の知覚情報が聴覚から得たものかを示します。
    const bool bHearing = _stimulus.Type == UAISense::GetSenseID<UAISenseConfig_Hearing>();

    //知覚が成功した場合の処理
    if (_stimulus.WasSuccessfullySensed())
    {
        m_lastKnownLocation = player->GetActorLocation();
        //プレイヤーが視覚的に知覚された場合、追跡を開始する
        if (bSight)
        {
            StartChase(player);
        }
        //プレイヤーが聴覚的に知覚された場合、追跡または探索を開始する
        else if (bHearing && m_currentState != EEnemyAIState::Chase && m_currentState != EEnemyAIState::Attack)
        {
            StartSearch(_stimulus.StimulusLocation);
        }
        return;
    }

    //知覚が失敗した場合の処理
    if (bSight && player == m_pSensedTarget)
    {
        //プレイヤーが視覚的に失われた場合、戦術的な追跡の更新を停止し、ターゲットをリセットして探索を開始する
        StopTacticalChaseUpdates();
        m_pSensedTarget = nullptr;
        m_currentManeuver = EEnemyCombatManeuver::None;
        ClearFocus(EAIFocusPriority::Gameplay);
        StartSearch(m_lastKnownLocation);
    }
}
