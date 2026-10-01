#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../../Player/PlayerChara.h"
#include "WeaponSlotEntryWidget.generated.h"

//UBorderは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UBorder;
//UImageは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UImage;
//UTextBlockは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UTextBlock;
//UTexture2Dは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UTexture2D;

//武器スロットEntryUIを表示するウィジェット
UCLASS()
class ZOMBIEATTACK_API UWeaponSlotEntryWidget : public UUserWidget
{
    GENERATED_BODY()

  public:
    //Widgetへ表示内容と強調状態を設定します。
    UFUNCTION(BlueprintCallable, Category = "Weapon UI")
    void Configure(EWeaponSlot _weaponSlot, const FText& _weaponName, UTexture2D* _weaponIcon, bool _bSelected, float _selectedOpacity,
                   float _inactiveOpacity, float _selectedScale, float _inactiveScale);

    //武器スロット表示へアイコン、名称、選択状態を設定します。
    UFUNCTION(BlueprintCallable, Category = "Weapon UI")
    void ConfigureEntry(EWeaponSlot _weaponSlot, const FText& _weaponName, UTexture2D* _weaponIcon, bool _bSelected);

  protected:
    //NativeConstructは、Widget構築時に必要な参照取得と初期表示を行います。
    virtual void NativeConstruct() override;

  private:
    //BP側のSelectionBorderを実行時に取得して選択色を更新します。
    UPROPERTY(Transient)
    TObjectPtr<UBorder> m_pSelectionBorder;

    //WeaponIconをゲーム処理から参照できるように管理します。
    UPROPERTY(Transient)
    TObjectPtr<UImage> m_pWeaponIcon;

    //WeaponNameTextをゲーム処理から参照できるように管理します。
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> m_pWeaponNameText;

  private:
    //武器スロットを保持します。
    UPROPERTY(VisibleAnywhere, Category = "Weapon UI")
    EWeaponSlot m_weaponSlot = EWeaponSlot::Pistol;

    //Selectedかを示します。
    UPROPERTY(VisibleAnywhere, Category = "Weapon UI")
    //Selectedかを示します。
    bool m_bSelected = false;
};
