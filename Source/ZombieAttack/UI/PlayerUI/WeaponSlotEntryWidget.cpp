#include "WeaponSlotEntryWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Blueprint/WidgetTree.h"

//Widgetが画面へ追加されたときに表示内容を初期化します。
void UWeaponSlotEntryWidget::NativeConstruct()
{
    //Widgetが画面へ追加されたときに表示内容を初期化します。
    Super::NativeConstruct();
    m_pSelectionBorder = WidgetTree ? Cast<UBorder>(WidgetTree->FindWidget(TEXT("SelectionBorder"))) : nullptr;
    m_pWeaponIcon = WidgetTree ? Cast<UImage>(WidgetTree->FindWidget(TEXT("WeaponIcon"))) : nullptr;
    m_pWeaponNameText = WidgetTree ? Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("WeaponNameText"))) : nullptr;
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
    //画面追加前の更新にも対応し、Blueprintの既存ウィジェット名から参照を取得する
    if (WidgetTree)
    {
        m_pSelectionBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("SelectionBorder")));
        m_pWeaponIcon = Cast<UImage>(WidgetTree->FindWidget(TEXT("WeaponIcon")));
        m_pWeaponNameText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("WeaponNameText")));
    }
    const float targetOpacity = _bSelected ? _selectedOpacity : _inactiveOpacity;
    const float targetScale = _bSelected ? _selectedScale : _inactiveScale;

    SetRenderOpacity(targetOpacity);
    SetRenderScale(FVector2D(targetScale, targetScale));
    if (m_pWeaponNameText)
    {
        m_pWeaponNameText->SetText(_weaponName);
        m_pWeaponNameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    }
    if (m_pWeaponIcon)
    {
        if (_weaponIcon)
        {
            m_pWeaponIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
            m_pWeaponIcon->SetBrushFromTexture(_weaponIcon, true);
            m_pWeaponIcon->SetDesiredSizeOverride(FVector2D(48.0f, 48.0f));
        }
        else
        {
            //FSlateBrush()を使うとSlateCoreリンクエラーになる場合があるため、非表示で対応します。
            m_pWeaponIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    if (m_pSelectionBorder)
    {
        const FLinearColor selectedColor(0.55f, 0.05f, 0.06f, 0.95f);
        const FLinearColor inactiveColor(0.02f, 0.02f, 0.02f, 0.35f);
        m_pSelectionBorder->SetBrushColor(_bSelected ? selectedColor : inactiveColor);
    }
}
