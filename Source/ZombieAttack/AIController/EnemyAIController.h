#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "EnemyAIController.generated.h"

//前方宣言
class UAIPerceptionComponent;
//UAISenseConfig_Sightは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAISenseConfig_Sight;
//UAISenseConfig_Hearingは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAISenseConfig_Hearing;

//敵AIの状態を表す列挙型
UENUM(BlueprintType)
enum class EEnemyAIState : uint8
{
    //決められた経路を巡回する
    Patrol,
    //最後に見つけた場所の周囲を探す
    Search,
    //見つけたプレイヤーを追跡する
    Chase,
    //攻撃できる距離で戦う
    Attack,
    //死亡後の行動を止める
    Dead
};

//敵の戦術的な行動を表す列挙型
UENUM(BlueprintType)
enum class EEnemyCombatManeuver : uint8
{
    //特別な移動を行わない
    None,
    //プレイヤーの進行先へ先回りする
    Intercept,
    //プレイヤーの左側へ回り込む
    FlankLeft,
    //プレイヤーの右側へ回り込む
    FlankRight,
    //攻撃範囲からいったん離れる
    Retreat
};

//敵AIControllerを制御するコントローラー
UCLASS()
class ZOMBIEATTACK_API AEnemyAIController : public AAIController
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //コンストラクタ
    AEnemyAIController();

    //追尾を開始する関数
    void StartChase(AActor* _target);

    //追尾を停止する関数
    void StopChase();

    //探索を開始する関数
    void StartSearch(const FVector& _searchLocation);

    //パトロールを開始する関数
    void StartRandomPatrol();

    //パトロールを停止する関数
    void StopPatrol();

    //攻撃が開始されたことを通知する関数
    virtual void NotifyAttackStarted();

    //攻撃が終了したことを通知する関数
    virtual void NotifyAttackFinished();

    //すべてのAIロジックを停止する関数
    virtual void StopAllLogic();

    //ターゲットが存在するかどうかを確認する関数
    bool HasTarget() const { return IsValid(m_pSensedTarget); }
    //発見時の咆哮が終わるまでは攻撃と移動の要求を待つ
    bool IsAlertReactionActive() const { return m_bAlertReactionActive; }
    //同時攻撃は二体までとし、同じ方向から仲間に重なって殴ることも避ける。
    bool HasAttackOpening() const;

    //現在のターゲットを取得する関数
    AActor* GetSensedActor() const { return m_pSensedTarget.Get(); }

    //現在のAI状態を取得する関数
    EEnemyAIState GetCurrentState() const { return m_currentState; }

    //現在の戦術的な行動を取得する関数
    UFUNCTION(BlueprintPure, Category = "AI|Combat")
    EEnemyCombatManeuver GetCurrentManeuver() const { return m_currentManeuver; }

  protected:
    //Possessされたときに呼び出される関数
    virtual void OnPossess(APawn* _inPawn) override;

    //UnPossessされたときに呼び出される関数
    virtual void OnUnPossess() override;

    //移動が完了したときに呼び出される関数
    virtual void OnMoveCompleted(FAIRequestID _requestID,
                                 //overrideをゲーム処理から参照できるように管理します。
                                 const FPathFollowingResult& _result) override;

    //戦術的な追跡を使用するかどうかを確認する関数
    virtual bool ShouldUseTacticalChase() const { return true; }

    //ターゲットの知覚が更新されたときに呼び出される関数
    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor* _actor, FAIStimulus _stimulus);

  private:
    //仲間が振りかぶり始めた直後に同時攻撃を重ねないための開始時刻。
    double m_lastAttackStart = -1000.0;
    //木の陰で見失うたびに咆哮を繰り返さないための再警戒までの間隔。
    UPROPERTY(EditDefaultsOnly, Category = "AI|Alert", meta = (ClampMin = "0.0"))
    float m_alertRepeatDelay = 10.0f;
    //直前に咆哮を開始した時刻。初回発見は必ず警戒動作を行う。
    float m_lastAlertTime = -1000.0f;
    //咆哮中に視線が切れた時、全身モーションを終了させてから探索へ戻す。
    void CancelAlertReaction();
    //パトロールの次のポイントをスケジュールする関数
    void ScheduleNextPatrol(float _overrideDelay = -1.f);

    //パトロールの次のポイントに移動する関数
    void MoveToNextPatrolPoint();

    //探索を終了する関数
    void FinishSearch();

    //現在のAI状態を設定する関数
    void SetState(EEnemyAIState _newState);

    //現在の戦術的な行動を設定する関数
    void StartTacticalChaseUpdates();

    //戦術的な追跡の更新を停止する関数
    void StopTacticalChaseUpdates();

    //戦術的な追跡を更新する関数
    void UpdateTacticalChase();

    //フランクの方向を更新する関数
    void RefreshFlankDirection();
    //ResumeChaseAfterAlertは、中断していた名前が示す動作を再開します。
    void ResumeChaseAfterAlert();

    //指定された位置をナビゲーションメッシュ上に投影する関数
    bool ProjectToNavigation(const FVector& _desiredLocation, FVector& _outProjectedLocation) const;

  private:
    //AIの知覚コンポーネント
    UPROPERTY(VisibleAnywhere, Category = "AI|Perception")
    TObjectPtr<UAIPerceptionComponent> m_pPerception;

    //視覚の知覚設定
    UPROPERTY()
    TObjectPtr<UAISenseConfig_Sight> m_pSightConfig;

    //聴覚の知覚設定
    UPROPERTY()
    TObjectPtr<UAISenseConfig_Hearing> m_pHearingConfig;

    //現在知覚しているターゲット
    UPROPERTY()
    TObjectPtr<AActor> m_pSensedTarget;

    //現在のAI状態
    UPROPERTY(VisibleAnywhere, Category = "AI")
    EEnemyAIState m_currentState;

    //現在の戦術的な行動
    UPROPERTY(VisibleAnywhere, Category = "AI|Combat")
    EEnemyCombatManeuver m_currentManeuver;

    //パトロールの半径
    UPROPERTY(EditAnywhere, Category = "AI|Patrol", meta = (ClampMin = "100.0"))
    float m_patrolRadius;

    //パトロールの最小移動距離
    UPROPERTY(EditAnywhere, Category = "AI|Patrol", meta = (ClampMin = "0.0"))
    float m_minPatrolMoveDistance;

    //パトロールの待機時間の最小値
    UPROPERTY(EditAnywhere, Category = "AI|Patrol", meta = (ClampMin = "0.0"))
    float m_patrolWaitMin;

    //パトロールの待機時間の最大値
    UPROPERTY(EditAnywhere, Category = "AI|Patrol", meta = (ClampMin = "0.0"))
    float m_patrolWaitMax;

    //探索の待機時間
    UPROPERTY(EditAnywhere, Category = "AI|Search", meta = (ClampMin = "0.0"))
    float m_searchLookDuration;

    //追跡の受け入れ半径
    UPROPERTY(EditAnywhere, Category = "AI|Chase", meta = (ClampMin = "5.0"))
    float m_chaseAcceptanceRadius;

    //戦術的な追跡の更新間隔
    UPROPERTY(EditAnywhere, Category = "AI|Tactical Chase", meta = (ClampMin = "0.1"))
    float m_tacticalUpdateInterval;

    //ターゲットの予測時間
    UPROPERTY(EditAnywhere, Category = "AI|Tactical Chase", meta = (ClampMin = "0.0"))
    float m_targetPredictionSeconds;

    //フランクの距離
    UPROPERTY(EditAnywhere, Category = "AI|Tactical Chase", meta = (ClampMin = "50.0"))
    float m_flankDistance;

    //後退の距離
    UPROPERTY(EditAnywhere, Category = "AI|Tactical Chase", meta = (ClampMin = "0.0"))
    float m_retreatDistance;

    //フランクの切り替えの最小時間
    UPROPERTY(EditAnywhere, Category = "AI|Tactical Chase", meta = (ClampMin = "0.1"))
    float m_flankSwitchMin;

    //フランクの切り替えの最大時間
    UPROPERTY(EditAnywhere, Category = "AI|Tactical Chase", meta = (ClampMin = "0.1"))
    float m_flankSwitchMax;

    //ナビゲーションメッシュ上に投影する際の範囲
    UPROPERTY(EditAnywhere, Category = "AI|Tactical Chase")
    FVector m_tacticalNavProjectionExtent;

    //タイマーのハンドル
    FTimerHandle m_patrolTimer;
    //searchTimerを秒単位で指定します。
    FTimerHandle m_searchTimer;
    //tacticalChaseTimerを秒単位で指定します。
    FTimerHandle m_tacticalChaseTimer;
    //flankSwitchTimerを秒単位で指定します。
    FTimerHandle m_flankSwitchTimer;
    //alertReactionTimerを秒単位で指定します。
    FTimerHandle m_alertReactionTimer;
    //lastKnown位置をゲーム処理から参照できるように管理します。
    FVector m_lastKnownLocation;

    //フランクの方向を示す符号（1または-1）
    float m_flankSign;
    //AlertReactionActiveかを示します。
    bool m_bAlertReactionActive;
};
