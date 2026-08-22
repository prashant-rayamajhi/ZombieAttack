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
    //「!PlayerController || !m_pUserWidget」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (!PlayerController || !m_pUserWidget) { return; }

    //クロスヘアは画面中央固定です。投影ヒット位置へ動かしません。
    //GunWeaponも画面中央からTraceするため、見た目と弾道が一致します。
    int32 ViewportX = 0;
    //ViewportYは、0から算出した数値を後続の判定または計算に使います。
    int32 ViewportY = 0;
    PlayerController->GetViewportSize(ViewportX, ViewportY);
    //「ViewportX <= 0 || ViewportY <= 0」が成立するとき、SetAimingを呼び出します。
    if (ViewportX <= 0 || ViewportY <= 0) { return; }

    m_pUserWidget->SetAiming(IsAiming());
}

//カメラが回転したときの入力値を受け取る関数

void APlayerChara::SwapCrosshairWidget()
{
    //「m_pUserWidget」が成立するとき、SetVisibilityを呼び出します。
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
    //「m_pUserWidget」が成立するとき、FlashKillConfirmationを呼び出します。
    if (m_pUserWidget)
    {
        m_pUserWidget->FlashKillConfirmation();
    }
}

//特定のレベルではHUDを非表示にする

void APlayerChara::ControlHUDVisiblity()
{
    //「!m_pUserWidget && !m_pPlayerHp && !m_pReloadUI」が成立するとき、GetCurrentLevelNameを呼び出します。
    if (!m_pUserWidget && !m_pPlayerHp && !m_pReloadUI) { return; }
    //lvは、UGameplayStatics::GetCurrentLevelName(GetWorld(), true)から構築した結果を後続の処理へ渡すために使います。
    FString lv = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
    //カットシーン中のHUDを非表示にするかを示します。
    bool bHide = lv.Equals(TEXT("GameClear"), ESearchCase::IgnoreCase) || lv.Equals(TEXT("GameStart"), ESearchCase::IgnoreCase) ||
                 lv.Equals(TEXT("GameOver"), ESearchCase::IgnoreCase);
    //「!bHide」が成立するとき、続けて「m_pUserWidget && m_pUserWidget->IsInViewport()」を判定します。
    if (!bHide) { return; }
    //「m_pUserWidget && m_pUserWidget->IsInViewport()」が成立するとき、RemoveFromParentを呼び出します。
    if (m_pUserWidget && m_pUserWidget->IsInViewport())
    {
        m_pUserWidget->RemoveFromParent();
        m_pUserWidget = nullptr;
    }
    //「m_pPlayerHp && m_pPlayerHp->IsInViewport()」が成立するとき、RemoveFromParentを呼び出します。
    if (m_pPlayerHp && m_pPlayerHp->IsInViewport())
    {
        m_pPlayerHp->RemoveFromParent();
        m_pPlayerHp = nullptr;
    }
    //「m_pReloadUI && m_pReloadUI->IsInViewport()」が成立するとき、RemoveFromParentを呼び出します。
    if (m_pReloadUI && m_pReloadUI->IsInViewport())
    {
        m_pReloadUI->RemoveFromParent();
        m_pReloadUI = nullptr;
    }
}

//足音の再生とAIへの通知

//アイテム取得イベントの呼び出し
