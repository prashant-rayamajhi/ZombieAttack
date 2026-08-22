#include "WeaponSlotEntryWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

//Widgetが画面へ追加されたときに表示内容を初期化します。
void UWeaponSlotEntryWidget::NativeConstruct()
{
    //Widgetが画面へ追加されたときに表示内容を初期化します。
    Super::NativeConstruct();
}

//武器スロット表示へアイコン、名称、選択状態を設定します。
void UWeaponSlotEntryWidget::ConfigureEntry(EWeaponSlot _weaponSlot, const FText& _weaponName, UTexture2D* _weaponIcon, bool _bSelected)
{
    Configure(_weaponSlot, _weaponName, _weaponIcon, _bSelected, 1.0f, 0.45f, 1.0f, 0.9f);
}

//Widgetへ表示内容と強調状態を設定します。
void UWeaponSlotEntryWidget::Configure(EWeaponSlot _weaponSlot, const FText& _weaponName, UTexture2D* _weaponIcon, bool _bSelected,
                                       float _selectedOpacity, float _inactiveOpacity, float _selectedScale, float _inactiveScale)
{
    m_weaponSlot = _weaponSlot;
    m_bSelected = _bSelected;

    //targetOpacityは、_bSelected ? _selectedOpacity : _inactiveOpacityから算出した数値を後続の判定または計算に使います。
    const float targetOpacity = _bSelected ? _selectedOpacity : _inactiveOpacity;
    //targetScaleは、_bSelected ? _selectedScale : _inactiveScaleから算出した数値を後続の判定または計算に使います。
    const float targetScale = _bSelected ? _selectedScale : _inactiveScale;

    SetRenderOpacity(targetOpacity);
    SetRenderScale(FVector2D(targetScale, targetScale));

    //「WeaponNameText」が成立するとき、SetTextを呼び出します。
    if (WeaponNameText)
    {
        WeaponNameText->SetText(_weaponName);
        WeaponNameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    }

    //「WeaponIcon」が成立するとき、続けて「_weaponIcon」を判定します。
    if (WeaponIcon)
    {
        //「_weaponIcon」が成立するとき、SetVisibilityを呼び出します。
        if (_weaponIcon)
        {
            WeaponIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
            WeaponIcon->SetBrushFromTexture(_weaponIcon, true);
            WeaponIcon->SetDesiredSizeOverride(FVector2D(48.0f, 48.0f));
        }
        else
        {
            //FSlateBrush()を使うとSlateCoreリンクエラーになる場合があるため、非表示で対応します。
            WeaponIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    //「SelectionBorder」が成立するとき、selectedColorを呼び出します。
    if (SelectionBorder)
    {
        //selectedColorは、0.55f, 0.05f, 0.06f, 0.95f)から構築した結果を後続の処理へ渡すために使います。
        const FLinearColor selectedColor(0.55f, 0.05f, 0.06f, 0.95f);
        //inactiveColorは、0.02f, 0.02f, 0.02f, 0.35f)から構築した結果を後続の処理へ渡すために使います。
        const FLinearColor inactiveColor(0.02f, 0.02f, 0.02f, 0.35f);
        SelectionBorder->SetBrushColor(_bSelected ? selectedColor : inactiveColor);
    }
}
