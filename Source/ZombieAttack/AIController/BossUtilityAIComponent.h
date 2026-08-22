#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossAITypes.h"
#include "BossUtilityAIComponent.generated.h"

//前方宣言
class ABossChara;
//APlayerCharaは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class APlayerChara;

//ボスUtilityAIComponentの処理を担当するコンポーネント
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class ZOMBIEATTACK_API UBossUtilityAIComponent : public UActorComponent
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //コンストラクタ
    UBossUtilityAIComponent();

    //アーキタイプに応じてパラメータを設定
    void ConfigureForArchetype(EBossAIArchetype _archetype);

    //ボスの意思決定コンテキストを構築
    FBossDecisionContext BuildDecisionContext(ABossChara* _boss, APlayerChara* _player, bool _bHasLineOfSight);

    //ボスの戦術行動を選択
    EBossTacticalAction ChooseAction(const FBossDecisionContext& _context,
                                     //constの操作に使用する参照です。
                                     const ABossChara* _boss) const;

    //ボスの戦術行動を記録
    void RecordAction(EBossTacticalAction _action);

    //ボスのダメージを通知
    void NotifyBossDamaged(float _damageAmount);

    //プレイヤーのダメージを通知
    void NotifyPlayerHit(float _damageAmount);

    //ボスの戦術行動のロック期間を取得
    float GetActionLockDuration(EBossTacticalAction _action) const;

  protected:
    //ボスの戦術行動のロック期間を設定
    UPROPERTY(EditAnywhere, Category = "Utility AI|Learning", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    //adaptationRateをゲーム処理から参照できるように管理します。
    float m_adaptationRate;

    //ボスの戦術行動のランダム性を設定
    UPROPERTY(EditAnywhere, Category = "Utility AI|Variation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    //randomnessをゲーム処理から参照できるように管理します。
    float m_randomness;

    UPROPERTY(EditAnywhere, Category = "Utility AI|Variation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    //repeatPenaltyをゲーム処理から参照できるように管理します。
    float m_repeatPenalty;

    //ボスの戦術行動の履歴長を設定
    UPROPERTY(EditAnywhere, Category = "Utility AI|Variation", meta = (ClampMin = "1", ClampMax = "8"))
    //historyLengthをゲーム処理から参照できるように管理します。
    int32 m_historyLength;

    //ボスの戦術行動の予測秒数を設定
    UPROPERTY(EditAnywhere, Category = "Utility AI|Prediction", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    //redictionSecondsの操作に使用する参照です。
    float m_predictionSeconds;

  private:
    //ボスの戦術行動のスコアを計算
    float ScoreAction(EBossTacticalAction _action, const FBossDecisionContext& _context,
                      //constの操作に使用する参照です。
                      const ABossChara* _boss) const;

    //ボスの戦術行動の履歴ペナルティを適用
    float ApplyHistoryPenalty(EBossTacticalAction _action, float _score) const;

    //ボスの戦術行動の履歴をカウント
    int32 CountRecentAction(EBossTacticalAction _action) const;

    //ボスの戦術行動の圧力を減衰
    void DecayTransientPressure(float _deltaSeconds);

  private:
    //ボスの戦術行動の履歴
    TArray<EBossTacticalAction> m_actionHistory;

    //ボスの戦術行動の圧力
    float m_rangedPressure;

    //ボスの戦術行動の機動性の圧力
    float m_mobilityPressure;

    //ボスの戦術行動の脆弱性の圧力
    float m_vulnerabilityPressure;

    //ボスの戦術行動の最近のダメージ圧力
    float m_recentBossDamagePressure;

    //ボスの戦術行動の最近のヒット圧力
    float m_recentSuccessfulHitPressure;

    //ボスの戦術行動の最終観察時間
    float m_lastObservationTime;
};
