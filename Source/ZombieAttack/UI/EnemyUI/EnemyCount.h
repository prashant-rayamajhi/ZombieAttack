#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyCount.generated.h"

//UTextBlockは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UTextBlock;
//UBorderは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UBorder;

//敵数の動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API UEnemyCount : public UUserWidget
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //NativeOnInitializedは、Widget初期化時にイベント接続と固定UI要素の準備を行います。
    virtual void NativeOnInitialized() override;
    //NativeTickは、Widgetの毎フレーム更新を受け取り、表示アニメーションを進めます。
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    //敵数UIを最新のゲーム状態へ更新します。
    UFUNCTION(BlueprintCallable, Category = "Enemy Count")
    void UpdateEnemyCountUI(int32 CurrentCount);

    //MissionObjectiveを画面へ表示します。
    UFUNCTION(BlueprintCallable, Category = "Mission")
    void ShowMissionObjective(int32 InitialEnemyCount);

    //ゴール準備完了を画面へ表示します。
    UFUNCTION(BlueprintCallable, Category = "Mission")
    void ShowGoalReady();

  protected:
    //enemy数Textを管理します。
    UPROPERTY(Transient)
    //enemy数Textを管理します。
    TObjectPtr<UTextBlock> m_enemyCountText;

  private:
    //ミッションHUDを作成します。
    void BuildMissionHUD();
    //ShowCenterMessageは、名前が示すUIを構築して画面へ表示します。
    void ShowCenterMessage(const FText& Header, const FText& Message, float Duration);

    //counterPanelを管理します。
    UPROPERTY(Transient)
    //counterPanelを管理します。
    TObjectPtr<UBorder> m_counterPanel;

    //missionPanelをゲーム処理から参照できるように管理します。
    UPROPERTY(Transient)
    //missionPanelをゲーム処理から参照できるように管理します。
    TObjectPtr<UBorder> m_missionPanel;

    //missionHeaderTextをゲーム処理から参照できるように管理します。
    UPROPERTY(Transient)
    //missionHeaderTextをゲーム処理から参照できるように管理します。
    TObjectPtr<UTextBlock> m_missionHeaderText;

    //missionMessageTextをゲーム処理から参照できるように管理します。
    UPROPERTY(Transient)
    //missionMessageTextをゲーム処理から参照できるように管理します。
    TObjectPtr<UTextBlock> m_missionMessageText;

    //mission数Textを管理します。
    UPROPERTY(Transient)
    //mission数Textを管理します。
    TObjectPtr<UTextBlock> m_missionCountText;

    //last敵数を管理します。
    int32 m_lastEnemyCount = INDEX_NONE;
    //counterPulse残りを管理します。
    float m_counterPulseRemaining = 0.0f;
    //missionTime残りを秒単位で指定します。
    float m_missionTimeRemaining = 0.0f;
    //mission時間を秒単位で指定します。
    float m_missionDuration = 0.0f;
    //counterRevealDelayを秒単位で指定します。
    float m_counterRevealDelay = 0.0f;
    //counterRevealElapsedを管理します。
    float m_counterRevealElapsed = 0.0f;
    //数erRevealPendingかを示します。
    bool m_bCounterRevealPending = false;
    //ObjectiveShownかを示します。
    bool m_bObjectiveShown = false;
};
