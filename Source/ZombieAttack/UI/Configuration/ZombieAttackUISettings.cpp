#include "ZombieAttackUISettings.h"

#include "../Ammo/AmmoHUDWidget.h"
#include "../EnemyUI/EnemyCount.h"
#include "../EnemyUI/EnemyLocatorWidget.h"
#include "../GameFlow/GameFlowScreenWidget.h"
#include "../PlayerUI/CombatCrosshairWidget.h"
#include "../PlayerUI/PlayerHP.h"

namespace
{
template <typename TWidget>
//ResolveWidgetClassは、ResolveWidgetClassの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
TSubclassOf<TWidget> ResolveWidgetClass(const TSoftClassPtr<TWidget>& _configuredClass, TSubclassOf<TWidget> _fallbackClass)
{
    //「UClass* loadedClass = _configuredClass.LoadSynchronous()」が成立するとき、この関数を終了します。
    if (UClass* loadedClass = _configuredClass.LoadSynchronous()) { return loadedClass; }

    //_fallbackClassは、生成または表示するUnrealクラスを指定するために使います。
    return _fallbackClass;
}
//名前空間を閉じます。
//名前空間を閉じます。
}

//UZombieAttackUISettingsが使用するComponentと初期パラメータを設定します。
UZombieAttackUISettings::UZombieAttackUISettings()
{
    m_gameStartWidgetClass =
        TSoftClassPtr<UGameFlowScreenWidget>(FSoftObjectPath(TEXT("/Game/Blueprints/UI/GameFlow/WBP_GameStartScreen.WBP_GameStartScreen_C")));
    m_gameClearWidgetClass =
        TSoftClassPtr<UGameFlowScreenWidget>(FSoftObjectPath(TEXT("/Game/Blueprints/UI/GameFlow/WBP_GameClearScreen.WBP_GameClearScreen_C")));
    m_gameOverWidgetClass =
        TSoftClassPtr<UGameFlowScreenWidget>(FSoftObjectPath(TEXT("/Game/Blueprints/UI/GameFlow/WBP_GameOverScreen.WBP_GameOverScreen_C")));

    m_playerHealthWidgetClass = TSoftClassPtr<UPlayerHP>(FSoftObjectPath(TEXT("/Game/Blueprints/UI/HUD/WBP_PlayerHealthHUD.WBP_PlayerHealthHUD_C")));
    m_ammoWidgetClass = TSoftClassPtr<UAmmoHUDWidget>(FSoftObjectPath(TEXT("/Game/Blueprints/UI/HUD/WBP_AmmoHUD.WBP_AmmoHUD_C")));
    m_crosshairWidgetClass =
        TSoftClassPtr<UCombatCrosshairWidget>(FSoftObjectPath(TEXT("/Game/Blueprints/UI/HUD/WBP_CombatCrosshair.WBP_CombatCrosshair_C")));
    m_enemyLocatorWidgetClass =
        TSoftClassPtr<UEnemyLocatorWidget>(FSoftObjectPath(TEXT("/Game/Blueprints/UI/HUD/WBP_EnemyLocator.WBP_EnemyLocator_C")));
    m_enemyCountWidgetClass = TSoftClassPtr<UEnemyCount>(FSoftObjectPath(TEXT("/Game/Blueprints/UI/HUD/WBP_EnemyMissionHUD.WBP_EnemyMissionHUD_C")));
}

//GameFlowWidgetクラスを取得して呼び出し元へ返します。
TSubclassOf<UGameFlowScreenWidget> UZombieAttackUISettings::GetGameFlowWidgetClass(const FString& _levelName) const
{
    //configuredClassは、&m_gameStartWidgetClassから取得した参照を後続の呼び出しで使います。
    const TSoftClassPtr<UGameFlowScreenWidget>* configuredClass = &m_gameStartWidgetClass;
    //「_levelName.Equals(TEXT("GameClear"), ESearchCase::IgnoreCase)」が成立するとき、configuredClassを更新します。
    if (_levelName.Equals(TEXT("GameClear"), ESearchCase::IgnoreCase))
    {
        configuredClass = &m_gameClearWidgetClass;
    }
    //GameOver画面の設定が必要か確認します。
    else if (_levelName.Equals(TEXT("GameOver"), ESearchCase::IgnoreCase))
    {
        configuredClass = &m_gameOverWidgetClass;
    }

    return ResolveWidgetClass<UGameFlowScreenWidget>(*configuredClass, TSubclassOf<UGameFlowScreenWidget>(UGameFlowScreenWidget::StaticClass()));
}

//プレイヤー体力Widgetクラスを取得して呼び出し元へ返します。
TSubclassOf<UPlayerHP> UZombieAttackUISettings::GetPlayerHealthWidgetClass() const
{
    return ResolveWidgetClass<UPlayerHP>(m_playerHealthWidgetClass, TSubclassOf<UPlayerHP>(UPlayerHP::StaticClass()));
}

//弾薬Widgetクラスを取得して呼び出し元へ返します。
TSubclassOf<UAmmoHUDWidget> UZombieAttackUISettings::GetAmmoWidgetClass() const
{
    return ResolveWidgetClass<UAmmoHUDWidget>(m_ammoWidgetClass, TSubclassOf<UAmmoHUDWidget>(UAmmoHUDWidget::StaticClass()));
}

//クロスヘアWidgetクラスを取得して呼び出し元へ返します。
TSubclassOf<UCombatCrosshairWidget> UZombieAttackUISettings::GetCrosshairWidgetClass() const
{
    return ResolveWidgetClass<UCombatCrosshairWidget>(m_crosshairWidgetClass,
                                                      TSubclassOf<UCombatCrosshairWidget>(UCombatCrosshairWidget::StaticClass()));
}

//敵LocatorWidgetクラスを取得して呼び出し元へ返します。
TSubclassOf<UEnemyLocatorWidget> UZombieAttackUISettings::GetEnemyLocatorWidgetClass() const
{
    return ResolveWidgetClass<UEnemyLocatorWidget>(m_enemyLocatorWidgetClass, TSubclassOf<UEnemyLocatorWidget>(UEnemyLocatorWidget::StaticClass()));
}

//残り敵数Widgetクラスを取得して呼び出し元へ返します。
TSubclassOf<UEnemyCount> UZombieAttackUISettings::GetEnemyCountWidgetClass() const
{
    return ResolveWidgetClass<UEnemyCount>(m_enemyCountWidgetClass, TSubclassOf<UEnemyCount>(UEnemyCount::StaticClass()));
}
