#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatCrosshairWidget.generated.h"

//画面中央に一つだけ表示する戦闘用クロスヘアです。
//通常・エイム・撃破確認を同じWidgetで描画し、BPの重複表示を防ぎます。
UCLASS()
class ZOMBIEATTACK_API UCombatCrosshairWidget : public UUserWidget
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //SetAimingは、引数の内容をゲーム中の状態または表示へ反映します。
    void SetAiming(bool _bAiming);
    //FlashKillConfirmationは、名前が示す通知を短時間の点滅表示で伝えます。
    void FlashKillConfirmation();

  protected:
    //NativeTickは、Widgetの毎フレーム更新を受け取り、表示アニメーションを進めます。
    virtual void NativeTick(const FGeometry& _geometry, float _deltaTime) override;

    virtual int32 NativePaint(const FPaintArgs& _args, const FGeometry& _geometry, const FSlateRect& _cullingRect,
                              FSlateWindowElementList& _drawElements, int32 _layerId, const FWidgetStyle& _widgetStyle,
                              //overrideかを示します。
                              bool _bParentEnabled) const override;

  private:
    //Aimingかを示します。
    bool m_bAiming = false;
    //killFlash残りをゲーム処理から参照できるように管理します。
    float m_killFlashRemaining = 0.0f;
};
