#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHP.generated.h"

//UProgressBarは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UProgressBar;
//UTextBlockは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UTextBlock;
//AEnemyCharaは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class AEnemyChara;

//敵体力の動作をまとめたクラス
UCLASS(Abstract)
class ZOMBIEATTACK_API UEnemyHP : public UUserWidget
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //体力UIを最新のゲーム状態へ更新します。
    UFUNCTION()
    void UpdateHealthUI();

    //所有者を設定します。
    void SetOwner(AEnemyChara* Enemy);

  protected:
    //healthBarをゲーム処理から参照できるように管理します。
    UPROPERTY(meta = (BindWidget))
    //healthBarをゲーム処理から参照できるように管理します。
    TObjectPtr<UProgressBar> m_healthBar;

    //現在の体力を保持します。
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> m_currentHealth;

    //最大体力を保持します。
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> m_maxHealth;

  private:
    //所有者を保持します。
    TWeakObjectPtr<AEnemyChara> m_owner;
};
