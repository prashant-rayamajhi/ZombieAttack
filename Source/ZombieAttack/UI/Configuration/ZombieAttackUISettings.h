#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ZombieAttackUISettings.generated.h"

//UAmmoHUDWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAmmoHUDWidget;
//UCombatCrosshairWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UCombatCrosshairWidget;
//UEnemyCountは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UEnemyCount;
//UEnemyLocatorWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UEnemyLocatorWidget;
//UGameFlowScreenWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UGameFlowScreenWidget;
//UPlayerHPは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UPlayerHP;

//画面ごとに使用するUIクラスを一元管理します。
//各画面はProject SettingsのZombie Attack UIから差し替えられます。
//ゲーム進行コードを変更せず調整でき、未設定時はC++製Widgetを代替表示します。
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Zombie Attack UI"))
class ZOMBIEATTACK_API UZombieAttackUISettings : public UDeveloperSettings
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    UZombieAttackUISettings();

    virtual FName GetCategoryName() const override { return TEXT("Game"); }

    //GetGameFlowWidgetClassは、呼び出し元が必要とする対象または計算結果を返します。
    TSubclassOf<UGameFlowScreenWidget> GetGameFlowWidgetClass(const FString& _levelName) const;
    //GetPlayerHealthWidgetClassは、呼び出し元が必要とする対象または計算結果を返します。
    TSubclassOf<UPlayerHP> GetPlayerHealthWidgetClass() const;
    //GetAmmoWidgetClassは、呼び出し元が必要とする対象または計算結果を返します。
    TSubclassOf<UAmmoHUDWidget> GetAmmoWidgetClass() const;
    //GetCrosshairWidgetClassは、呼び出し元が必要とする対象または計算結果を返します。
    TSubclassOf<UCombatCrosshairWidget> GetCrosshairWidgetClass() const;
    //GetEnemyLocatorWidgetClassは、呼び出し元が必要とする対象または計算結果を返します。
    TSubclassOf<UEnemyLocatorWidget> GetEnemyLocatorWidgetClass() const;
    //GetEnemyCountWidgetClassは、呼び出し元が必要とする対象または計算結果を返します。
    TSubclassOf<UEnemyCount> GetEnemyCountWidgetClass() const;

  private:
    //gameStartWidgetクラスをゲーム処理から参照できるように管理します。
    UPROPERTY(Config, EditAnywhere, Category = "Game Flow")
    //gameStartWidgetクラスをゲーム処理から参照できるように管理します。
    TSoftClassPtr<UGameFlowScreenWidget> m_gameStartWidgetClass;

    //gameClearWidgetクラスをゲーム処理から参照できるように管理します。
    UPROPERTY(Config, EditAnywhere, Category = "Game Flow")
    //gameClearWidgetクラスをゲーム処理から参照できるように管理します。
    TSoftClassPtr<UGameFlowScreenWidget> m_gameClearWidgetClass;

    //gameOverWidgetクラスをゲーム処理から参照できるように管理します。
    UPROPERTY(Config, EditAnywhere, Category = "Game Flow")
    //gameOverWidgetクラスをゲーム処理から参照できるように管理します。
    TSoftClassPtr<UGameFlowScreenWidget> m_gameOverWidgetClass;

    //layer体力Widgetクラスの操作に使用する参照です。
    UPROPERTY(Config, EditAnywhere, Category = "In Game HUD")
    //layer体力Widgetクラスの操作に使用する参照です。
    TSoftClassPtr<UPlayerHP> m_playerHealthWidgetClass;

    //ammoWidgetクラスをゲーム処理から参照できるように管理します。
    UPROPERTY(Config, EditAnywhere, Category = "In Game HUD")
    //ammoWidgetクラスをゲーム処理から参照できるように管理します。
    TSoftClassPtr<UAmmoHUDWidget> m_ammoWidgetClass;

    //crosshairWidgetクラスをゲーム処理から参照できるように管理します。
    UPROPERTY(Config, EditAnywhere, Category = "In Game HUD")
    //crosshairWidgetクラスをゲーム処理から参照できるように管理します。
    TSoftClassPtr<UCombatCrosshairWidget> m_crosshairWidgetClass;

    //enemyLocatorWidgetクラスをゲーム処理から参照できるように管理します。
    UPROPERTY(Config, EditAnywhere, Category = "In Game HUD")
    //enemyLocatorWidgetクラスをゲーム処理から参照できるように管理します。
    TSoftClassPtr<UEnemyLocatorWidget> m_enemyLocatorWidgetClass;

    //enemy数Widgetクラスを管理します。
    UPROPERTY(Config, EditAnywhere, Category = "In Game HUD")
    //enemy数Widgetクラスを管理します。
    TSoftClassPtr<UEnemyCount> m_enemyCountWidgetClass;
};
