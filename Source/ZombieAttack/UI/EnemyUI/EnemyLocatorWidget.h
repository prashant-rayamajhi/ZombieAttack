#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyLocatorWidget.generated.h"

//見失った敵と解放された出口を、種類・距離・画面外の方向で案内する。
//アウトライン判定はEnemyCharaが担当し、このWidgetは最寄り対象の表示だけを担当します。
UCLASS()
class ZOMBIEATTACK_API UEnemyLocatorWidget : public UUserWidget
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  protected:
    virtual int32 NativePaint(const FPaintArgs& _args, const FGeometry& _geometry, const FSlateRect& _cullingRect,
                              FSlateWindowElementList& _drawElements, int32 _layerId, const FWidgetStyle& _widgetStyle,
                              bool _bParentEnabled) const override;
};
