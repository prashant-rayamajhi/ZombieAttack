#include "WeaponCarouselWidget.h"

#include "WeaponSlotEntryWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

//Widgetが画面へ追加されたときに表示内容を初期化します。
void UWeaponCarouselWidget::NativeConstruct()
{
    //Widgetが画面へ追加されたときに表示内容を初期化します。
    Super::NativeConstruct();
    m_pWeaponListBox = WidgetTree ? Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("WeaponListBox"))) : nullptr;

    //BP内に古い中央寄せ座標が残っていても、敵数・HPと同じ左端へ揃えます。
    if (m_pWeaponListBox && WidgetTree && WidgetTree->RootWidget)
    {
        UWidget* layoutRoot = m_pWeaponListBox;
        //武器スロットの表示数が上限を超えない間、次の武器情報を一覧へ追加します。
        while (layoutRoot->GetParent() && layoutRoot->GetParent() != WidgetTree->RootWidget)
        {
            layoutRoot = layoutRoot->GetParent();
        }
        if (UCanvasPanelSlot* canvasSlot = Cast<UCanvasPanelSlot>(layoutRoot->Slot))
        {
            canvasSlot->SetAnchors(FAnchors(0.0f, 1.0f));
            canvasSlot->SetAlignment(FVector2D(0.0f, 1.0f));
            canvasSlot->SetAutoSize(true);
            //画面上の配置はPlayerChara側で決め、内側でも同じ余白を加算しない。
            canvasSlot->SetPosition(FVector2D::ZeroVector);
        }
        else
        {
            m_pWeaponListBox->SetRenderTranslation(FVector2D::ZeroVector);
        }
    }
}

//WeaponOrderを現在の装備と進行状況に合わせて更新します。
void UWeaponCarouselWidget::RefreshWeaponOrder(const TArray<EWeaponSlot>& _weaponSlots, EWeaponSlot _currentSlot)
{
    if (!m_pWeaponListBox)
    {
        return;
    }

    if (!m_entryWidgetClass)
    {
        return;
    }

    m_pWeaponListBox->ClearChildren();

    if (_weaponSlots.Num() == 0)
    {
        return;
    }
    TArray<EWeaponSlot> displaySlots;
    for (const EWeaponSlot weaponSlot : _weaponSlots)
    {
        if (weaponSlot != _currentSlot)
        {
            displaySlots.Add(weaponSlot);
        }
    }

    displaySlots.Add(_currentSlot);
    for (const EWeaponSlot weaponSlot : displaySlots)
    {
        UWeaponSlotEntryWidget* entryWidget = CreateWidget<UWeaponSlotEntryWidget>(GetOwningPlayer(), m_entryWidgetClass);

        if (!entryWidget)
        {
            continue;
        }

        //この武器スロットが現在選択されているかを示します。
        const bool bSelected = (weaponSlot == _currentSlot);

        //子Widgetの構築でImageとTextBlockの参照が揃ってから、武器の表示内容を渡す
        UVerticalBoxSlot* verticalBoxSlot = m_pWeaponListBox->AddChildToVerticalBox(entryWidget);
        entryWidget->Configure(weaponSlot, GetNameForSlot(weaponSlot), GetIconForSlot(weaponSlot), bSelected, m_selectedOpacity, m_inactiveOpacity,
                               m_selectedScale, m_inactiveScale);
        if (verticalBoxSlot)
        {
            verticalBoxSlot->SetPadding(FMargin(0.0f, 4.0f));
            verticalBoxSlot->SetHorizontalAlignment(HAlign_Left);
        }
    }
}

//IconForSlotを取得して呼び出し元へ返します。
UTexture2D* UWeaponCarouselWidget::GetIconForSlot(EWeaponSlot _weaponSlot) const
{
    //BPに残っている旧設定を使わず、生成済みの画像を直接参照します。
    static const TSoftObjectPtr<UTexture2D> PistolIcon(FSoftObjectPath(TEXT("/Game/UI/WeaponIcons/T_Pistol_Icon.T_Pistol_Icon")));
    static const TSoftObjectPtr<UTexture2D> ARIcon(FSoftObjectPath(TEXT("/Game/UI/WeaponIcons/T_AR_Icon.T_AR_Icon")));
    static const TSoftObjectPtr<UTexture2D> KnifeIcon(FSoftObjectPath(TEXT("/Game/UI/WeaponIcons/T_Knife_Icon.T_Knife_Icon")));
    switch (_weaponSlot)
    {
    case EWeaponSlot::Pistol: return PistolIcon.LoadSynchronous();

    case EWeaponSlot::AR: return ARIcon.LoadSynchronous();

    case EWeaponSlot::Knife: return KnifeIcon.LoadSynchronous();

    default: return nullptr;
    }
}

//NameForSlotを取得して呼び出し元へ返します。
FText UWeaponCarouselWidget::GetNameForSlot(EWeaponSlot _weaponSlot) const
{
    switch (_weaponSlot)
    {
    case EWeaponSlot::Pistol: return FText::FromString(TEXT("PISTOL"));

    case EWeaponSlot::AR: return FText::FromString(TEXT("AR"));

    case EWeaponSlot::Knife: return FText::FromString(TEXT("KNIFE"));

    default: return FText::FromString(TEXT("UNKNOWN"));
    }
}
