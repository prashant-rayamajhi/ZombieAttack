#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AmmoHUDWidget.generated.h"

//AGunWeaponは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class AGunWeapon;
//UAmmoRadialWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAmmoRadialWidget;
//UTextBlockは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UTextBlock;

//C++だけで生成される二重円形弾数HUDです。
UCLASS()
class ZOMBIEATTACK_API UAmmoHUDWidget : public UUserWidget
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //武器を設定します。
    void SetWeapon(AGunWeapon* _gunWeapon);

  protected:
    //NativeOnInitializedは、Widget初期化時にイベント接続と固定UI要素の準備を行います。
    virtual void NativeOnInitialized() override;
    //NativeDestructは、Widget破棄時にイベント接続を解除し、残った参照を片付けます。
    virtual void NativeDestruct() override;

  private:
    //弾薬を更新します。
    UFUNCTION()
    void UpdateAmmo();
    void UnbindWeapon();

  private:
    //RadialWidgetの操作に使用する参照です。
    UPROPERTY(Transient)
    //RadialWidgetの操作に使用する参照です。
    TObjectPtr<UAmmoRadialWidget> m_pRadialWidget;

    //ClipTextの操作に使用する参照です。
    UPROPERTY(Transient)
    //ClipTextの操作に使用する参照です。
    TObjectPtr<UTextBlock> m_pClipText;

    //ReserveTextの操作に使用する参照です。
    UPROPERTY(Transient)
    //ReserveTextの操作に使用する参照です。
    TObjectPtr<UTextBlock> m_pReserveText;

    //武器を保持します。
    TWeakObjectPtr<AGunWeapon> m_pWeapon;
};
