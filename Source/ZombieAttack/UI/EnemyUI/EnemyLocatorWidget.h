#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyLocatorWidget.generated.h"

//30秒以上見つけられていない敵の方向を、画面端の赤い矢印で案内します。
//アウトライン判定はEnemyCharaが担当し、このWidgetは最寄り対象の表示だけを担当します。
UCLASS()
class ZOMBIEATTACK_API UEnemyLocatorWidget : public UUserWidget
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                              FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
                              //overrideかを示します。
                              bool bParentEnabled) const override;
};
