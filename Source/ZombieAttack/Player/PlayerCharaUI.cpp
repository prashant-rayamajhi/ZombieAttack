#include "PlayerChara.h"
#include "../UI/Ammo/AmmoHUDWidget.h"
#include "../UI/PlayerUI/CombatCrosshairWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

//クロスヘアを更新します。
void APlayerChara::UpdateCrosshair(float _deltaTime)
{
    (void)_deltaTime;
    //コントローラーを返します。
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController || !m_pUserWidget) { return; }

    //クロスヘアは画面中央固定です。投影ヒット位置へ動かしません。
    //GunWeaponも画面中央からTraceするため、見た目と弾道が一致します。
    int32 ViewportX = 0;
    int32 ViewportY = 0;
    PlayerController->GetViewportSize(ViewportX, ViewportY);
    if (ViewportX <= 0 || ViewportY <= 0) { return; }

    m_pUserWidget->SetAiming(IsAiming());
}

//カメラが回転したときの入力値を受け取る関数

void APlayerChara::SwapCrosshairWidget()
{
    if (m_pUserWidget)
    {
        //表示状態を設定します。
        m_pUserWidget->SetVisibility(m_bCrosshairSuppressed ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        m_pUserWidget->SetAiming(IsAiming());
    }
}

//クロスヘア非表示状態を設定します。
void APlayerChara::SetCrosshairSuppressed(bool _bSuppressed)
{
    m_bCrosshairSuppressed = _bSuppressed;
    SwapCrosshairWidget();
}

//敵を倒したことをゲーム側へ通知します。
void APlayerChara::NotifyEnemyDefeated()
{
    if (m_pUserWidget)
    {
        m_pUserWidget->FlashKillConfirmation();
    }
}

//特定のレベルではHUDを非表示にする

void APlayerChara::ControlHUDVisiblity()
{
    if (!m_pUserWidget && !m_pPlayerHp && !m_pReloadUI) { return; }
    const FString levelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
    //カットシーン中のHUDを非表示にするかを示します。
    const bool bHide = levelName.Equals(TEXT("GameClear"), ESearchCase::IgnoreCase) ||
                       levelName.Equals(TEXT("GameStart"), ESearchCase::IgnoreCase) || levelName.Equals(TEXT("GameOver"), ESearchCase::IgnoreCase);
    if (!bHide) { return; }
    if (m_pUserWidget && m_pUserWidget->IsInViewport())
    {
        m_pUserWidget->RemoveFromParent();
        m_pUserWidget = nullptr;
    }
    if (m_pPlayerHp && m_pPlayerHp->IsInViewport())
    {
        m_pPlayerHp->RemoveFromParent();
        m_pPlayerHp = nullptr;
    }
    if (m_pReloadUI && m_pReloadUI->IsInViewport())
    {
        m_pReloadUI->RemoveFromParent();
        m_pReloadUI = nullptr;
    }
}

//足音の再生とAIへの通知

//アイテム取得イベントの呼び出し
