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

    //BP内に古い中央寄せ座標が残っていても、敵数・HPと同じ左端へ揃えます。
    if (WeaponListBox && WidgetTree && WidgetTree->RootWidget)
    {
        //layoutRootは、WeaponListBoxから取得した参照を後続の呼び出しで使います。
        UWidget* layoutRoot = WeaponListBox;
        //武器スロットの表示数が上限を超えない間、次の武器情報を一覧へ追加します。
        while (layoutRoot->GetParent() && layoutRoot->GetParent() != WidgetTree->RootWidget)
        {
            layoutRoot = layoutRoot->GetParent();
        }

        //「UCanvasPanelSlot* canvasSlot = Cast<UCanvasPanelSlot>(layoutRoot->Slot)」が成立するとき、SetAnchorsを呼び出します。
        if (UCanvasPanelSlot* canvasSlot = Cast<UCanvasPanelSlot>(layoutRoot->Slot))
        {
            canvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
            canvasSlot->SetAlignment(FVector2D::ZeroVector);
            canvasSlot->SetPosition(FVector2D(0.0f, 158.0f));
        }
        else
        {
            WeaponListBox->SetRenderTranslation(FVector2D(-92.0f, 150.0f));
        }
    }
}

//WeaponOrderを現在の装備と進行状況に合わせて更新します。
void UWeaponCarouselWidget::RefreshWeaponOrder(const TArray<EWeaponSlot>& _weaponSlots, EWeaponSlot _currentSlot)
{
    if (!WeaponListBox)
    {
        return;
    }

    if (!m_entryWidgetClass)
    {
        return;
    }

    WeaponListBox->ClearChildren();

    if (_weaponSlots.Num() == 0)
    {
        return;
    }

    //displaySlotsは、直後の初期化結果を、同じスコープ内でこの名前を参照する計算や関数呼び出しへ渡すために使います。
    TArray<EWeaponSlot> displaySlots;

    //「const EWeaponSlot weaponSlot : _weaponSlots」の範囲を走査し、続けて「weaponSlot != _currentSlot」を判定します。
    for (const EWeaponSlot weaponSlot : _weaponSlots)
    {
        //「weaponSlot != _currentSlot」が成立するとき、Addを呼び出します。
        if (weaponSlot != _currentSlot)
        {
            displaySlots.Add(weaponSlot);
        }
    }

    displaySlots.Add(_currentSlot);

    //「const EWeaponSlot weaponSlot : displaySlots」の範囲を走査し、GetOwningPlayerを呼び出します。
    for (const EWeaponSlot weaponSlot : displaySlots)
    {
        //entryWidgetは、CreateWidget<UWeaponSlotEntryWidget>(GetOwningPlayer(), m_entryWidgetCl…から取得した参照を後続の呼び出しで使います。
        UWeaponSlotEntryWidget* entryWidget = CreateWidget<UWeaponSlotEntryWidget>(GetOwningPlayer(), m_entryWidgetClass);

        if (!entryWidget)
        {
            continue;
        }

        //この武器スロットが現在選択されているかを示します。
        const bool bSelected = (weaponSlot == _currentSlot);

        entryWidget->Configure(weaponSlot, GetNameForSlot(weaponSlot), GetIconForSlot(weaponSlot), bSelected, m_selectedOpacity, m_inactiveOpacity,
                               m_selectedScale, m_inactiveScale);

        //verticalBoxSlotは、WeaponListBox->AddChildToVerticalBox(entryWidget)から取得した参照を後続の呼び出しで使います。
        UVerticalBoxSlot* verticalBoxSlot = WeaponListBox->AddChildToVerticalBox(entryWidget);

        //「verticalBoxSlot」が成立するとき、SetPaddingを呼び出します。
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
    //ARIconは、ARIconの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    static const TSoftObjectPtr<UTexture2D> ARIcon(FSoftObjectPath(TEXT("/Game/UI/WeaponIcons/T_AR_Icon.T_AR_Icon")));
    //KnifeIconは、KnifeIconの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    static const TSoftObjectPtr<UTexture2D> KnifeIcon(FSoftObjectPath(TEXT("/Game/UI/WeaponIcons/T_Knife_Icon.T_Knife_Icon")));

    //現在の状態に合う処理へ分けます。
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
    //現在の状態に合う処理へ分けます。
    switch (_weaponSlot)
    {
    case EWeaponSlot::Pistol: return FText::FromString(TEXT("PISTOL"));

    case EWeaponSlot::AR: return FText::FromString(TEXT("AR"));

    case EWeaponSlot::Knife: return FText::FromString(TEXT("KNIFE"));

    default: return FText::FromString(TEXT("UNKNOWN"));
    }
}
