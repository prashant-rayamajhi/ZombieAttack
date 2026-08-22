#pragma once

#include "CoreMinimal.h"
#include "EnemyAIController.h"
#include "BossAITypes.h"
#include "BossAIController.generated.h"

//前方宣言
class UBossUtilityAIComponent;
//ABossCharaは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class ABossChara;
//APlayerCharaは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class APlayerChara;

//ボスAIControllerを制御するコントローラー
UCLASS()
class ZOMBIEATTACK_API ABossAIController : public AEnemyAIController
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //コンストラクタ
    ABossAIController();

    //ボスがダメージを受けたことを通知する
    void NotifyBossDamaged(float _damageAmount);

    //プレイヤーがダメージを受けたことを通知する
    void NotifyPlayerHit(float _damageAmount);

    //攻撃が終了したことを通知する
    virtual void NotifyAttackFinished() override;

    //すべてのロジックを停止する
    virtual void StopAllLogic() override;

    //現在の戦術行動を取得する
    UFUNCTION(BlueprintPure, Category = "Boss AI")
    EBossTacticalAction GetCurrentTacticalAction() const { return m_currentAction; }

  protected:
    //ポーンを所持したときの処理
    virtual void OnPossess(APawn* _inPawn) override;

    //ポーンの所持を解除したときの処理
    virtual void OnUnPossess() override;

    //戦術的な追跡を使用するかどうかを決定する
    virtual bool ShouldUseTacticalChase() const override { return false; }

  private:
    //次の意思決定をスケジュールする
    void ScheduleNextDecision(float _overrideDelay = -1.f);

    //意思決定を評価する
    void EvaluateDecision();

    //戦術行動を実行する
    bool ExecuteAction(EBossTacticalAction _action, ABossChara* _boss, APlayerChara* _player, const FBossDecisionContext& _context);

    //戦術的な位置に移動する
    bool MoveToTacticalLocation(const FVector& _desiredLocation, float _acceptanceRadius);

  private:
    //ユーティリティAIコンポーネント
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss AI", meta = (AllowPrivateAccess = "true"))
    //UtilityAIの操作に使用する参照です。
    TObjectPtr<UBossUtilityAIComponent> m_pUtilityAI;

    //現在の戦術行動
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss AI", meta = (AllowPrivateAccess = "true"))
    //currentActionをゲーム処理から参照できるように管理します。
    EBossTacticalAction m_currentAction;

    //アーキタイププリセットを使用するかどうか
    UPROPERTY(EditAnywhere, Category = "Boss AI|Decision")
    bool m_bUseArchetypePreset;

    //意思決定の間隔（秒）
    UPROPERTY(EditAnywhere, Category = "Boss AI|Decision", meta = (ClampMin = "0.1"))
    float m_decisionIntervalMin;

    //意思決定の間隔の最大値（秒）
    UPROPERTY(EditAnywhere, Category = "Boss AI|Decision", meta = (ClampMin = "0.1"))
    float m_decisionIntervalMax;

    //戦術行動の実行に必要な最小距離（秒）
    UPROPERTY(EditAnywhere, Category = "Boss AI|Movement", meta = (ClampMin = "50.0"))
    float m_approachStandOffDistance;

    //回避行動の半径（秒）
    UPROPERTY(EditAnywhere, Category = "Boss AI|Movement", meta = (ClampMin = "50.0"))
    float m_circleRadius;

    //回避行動の側面オフセット（秒）
    UPROPERTY(EditAnywhere, Category = "Boss AI|Movement", meta = (ClampMin = "0.0"))
    float m_circleSideOffset;

    //後退行動の距離（秒）
    UPROPERTY(EditAnywhere, Category = "Boss AI|Movement", meta = (ClampMin = "0.0"))
    float m_bossRetreatDistance;

    //ナビゲーションメッシュ上に投影する際の範囲
    UPROPERTY(EditAnywhere, Category = "Boss AI|Movement")
    FVector m_navProjectionExtent;


    //意思決定タイマー
    FTimerHandle m_decisionTimer;
    //nextAllowedDecisionTimeを秒単位で指定します。
    float m_nextAllowedDecisionTime;
};
