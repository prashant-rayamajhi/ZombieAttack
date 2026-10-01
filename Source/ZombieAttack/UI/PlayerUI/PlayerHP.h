#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHP.generated.h"

//UProgressBarは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UProgressBar;
//UTextBlockは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UTextBlock;
//UBorderは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UBorder;
//APlayerCharaは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class APlayerChara;

//プレイヤー体力の動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API UPlayerHP : public UUserWidget
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //体力UIを最新のゲーム状態へ更新します。
    UFUNCTION()
    void UpdateHealthUI();

    //所有者を設定します。
    void SetOwner(APlayerChara* _pPlayer);

  protected:
    //NativeOnInitializedは、Widget初期化時にイベント接続と固定UI要素の準備を行います。
    virtual void NativeOnInitialized() override;
    //NativeTickは、Widgetの毎フレーム更新を受け取り、表示アニメーションを進めます。
    virtual void NativeTick(const FGeometry& _geometry, float _deltaTime) override;

    //体力HUDを作成します。
    void BuildHealthHUD();

    //healthBarをゲーム処理から参照できるように管理します。
    UPROPERTY(meta = (BindWidgetOptional))
    //healthBarをゲーム処理から参照できるように管理します。
    TObjectPtr<UProgressBar> m_healthBar;

    //現在の体力を保持します。
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> m_currentHealth;

    //最大体力を保持します。
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> m_maxHealth;

    //体力値を保持します。
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> m_healthValue;

    //healthPanelをゲーム処理から参照できるように管理します。
    UPROPERTY(Transient)
    //healthPanelをゲーム処理から参照できるように管理します。
    TObjectPtr<UBorder> m_healthPanel;

    //damage画面Overlayをゲーム処理から参照できるように管理します。
    UPROPERTY(Transient)
    //damage画面Overlayをゲーム処理から参照できるように管理します。
    TObjectPtr<UBorder> m_damageScreenOverlay;

  private:
    //所有者を保持します。
    TWeakObjectPtr<APlayerChara> m_owner;
    //last体力Ratioの調整値です。
    float m_lastHealthRatio = 1.0f;
    //displayed体力Ratioの調整値です。
    float m_displayedHealthRatio = 1.0f;
    //target体力Ratioの調整値です。
    float m_targetHealthRatio = 1.0f;
    //damagePulse残りをゲーム処理から参照できるように管理します。
    float m_damagePulseRemaining = 0.0f;
    //HealingPulseかを示します。
    bool m_bHealingPulse = false;
};
