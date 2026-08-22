#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../../Player/PlayerChara.h"
#include "WeaponCarouselWidget.generated.h"

//UVerticalBoxは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UVerticalBox;
//UTexture2Dは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UTexture2D;
//UWeaponSlotEntryWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UWeaponSlotEntryWidget;

//武器CarouselUIを表示するウィジェット
UCLASS()
class ZOMBIEATTACK_API UWeaponCarouselWidget : public UUserWidget
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //WeaponOrderを現在の装備と進行状況に合わせて更新します。
    UFUNCTION(BlueprintCallable, Category = "Weapon UI")
    void RefreshWeaponOrder(const TArray<EWeaponSlot>& _weaponSlots, EWeaponSlot _currentSlot);

  protected:
    //NativeConstructは、Widget構築時に必要な参照取得と初期表示を行います。
    virtual void NativeConstruct() override;

  private:
    //BindWidgetはBP側の名前と完全一致が必要です。
    //WBP_WeaponCarousel側のVerticalBox名をWeaponListBoxにしてください。
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> WeaponListBox;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon UI", meta = (AllowPrivateAccess = "true", DisplayName = "Entry Widget Class"))
    //entryWidgetクラスをゲーム処理から参照できるように管理します。
    TSubclassOf<UWeaponSlotEntryWidget> m_entryWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon UI", meta = (AllowPrivateAccess = "true", DisplayName = "Pistol Icon"))
    //istolIconの操作に使用する参照です。
    TObjectPtr<UTexture2D> m_pistolIcon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon UI", meta = (AllowPrivateAccess = "true", DisplayName = "AR Icon"))
    //arIconをゲーム処理から参照できるように管理します。
    TObjectPtr<UTexture2D> m_arIcon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon UI", meta = (AllowPrivateAccess = "true", DisplayName = "Knife Icon"))
    //knifeIconをゲーム処理から参照できるように管理します。
    TObjectPtr<UTexture2D> m_knifeIcon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon UI", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
    //selected透明度をゲーム処理から参照できるように管理します。
    float m_selectedOpacity = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon UI", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
    //inactive透明度をゲーム処理から参照できるように管理します。
    float m_inactiveOpacity = 0.45f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon UI", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
    //selectedScaleをゲーム処理から参照できるように管理します。
    float m_selectedScale = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon UI", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
    //inactiveScaleをゲーム処理から参照できるように管理します。
    float m_inactiveScale = 0.9f;

  private:
    //GetIconForSlotは、呼び出し元が必要とする対象または計算結果を返します。
    UTexture2D* GetIconForSlot(EWeaponSlot _weaponSlot) const;
    //GetNameForSlotは、呼び出し元が必要とする対象または計算結果を返します。
    FText GetNameForSlot(EWeaponSlot _weaponSlot) const;
};
