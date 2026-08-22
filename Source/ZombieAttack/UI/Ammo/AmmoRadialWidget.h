#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "AmmoRadialWidget.generated.h"

//SAmmoRadialは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class SAmmoRadial;

//弾倉と予備弾薬を二重円で描画するコードネイティブWidget。
//内円は CurrentClip / MaxClip、外円は Reserve / MaxReserve を表します。
UCLASS()
class ZOMBIEATTACK_API UAmmoRadialWidget : public UWidget
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    UAmmoRadialWidget();

    //SetAmmoPercentは、引数の内容をゲーム中の状態または表示へ反映します。
    void SetAmmoPercent(float _clipPercent, float _reservePercent);

  protected:
    //RebuildWidgetは、Slate Widgetを組み立て直し、UMGが描画できるルートを返します。
    virtual TSharedRef<SWidget> RebuildWidget() override;
    //ReleaseSlateResourcesは、Slateの描画リソースを解放し、破棄後の参照保持を防ぎます。
    virtual void ReleaseSlateResources(bool _bReleaseChildren) override;
    //SynchronizePropertiesは、UMGプロパティの変更をSlate側の描画状態へ反映します。
    virtual void SynchronizeProperties() override;

  private:
    //clip色をゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Ammo Ring")
    //clip色をゲーム処理から参照できるように管理します。
    FLinearColor m_clipColor;

    //reserve色をゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Ammo Ring")
    //reserve色をゲーム処理から参照できるように管理します。
    FLinearColor m_reserveColor;

    //empty色をゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Ammo Ring")
    //empty色をゲーム処理から参照できるように管理します。
    FLinearColor m_emptyColor;

    //ringThicknessをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Ammo Ring")
    //ringThicknessをゲーム処理から参照できるように管理します。
    float m_ringThickness;

    //clipPercentをゲーム処理から参照できるように管理します。
    float m_clipPercent;
    //reservePercentをゲーム処理から参照できるように管理します。
    float m_reservePercent;
    //SlateWidgetの操作に使用する参照です。
    TSharedPtr<SAmmoRadial> m_pSlateWidget;
};
