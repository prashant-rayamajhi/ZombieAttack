#include "AmmoHUDWidget.h"

#include "AmmoRadialWidget.h"
#include "../../Weapon/GunWeapon.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

//Widget生成時に子Widgetとゲーム側の通知を接続します。
void UAmmoHUDWidget::NativeOnInitialized()
{
    //Widget生成時に子Widgetとゲーム側の通知を接続します。
    Super::NativeOnInitialized();

    //DesignerRadialは、WidgetTree ? Cast<UAmmoRadialWidget>(WidgetTree->FindWidget(TEXT("AmmoR…から取得した参照を後続の呼び出しで使います。
    UAmmoRadialWidget* DesignerRadial = WidgetTree ? Cast<UAmmoRadialWidget>(WidgetTree->FindWidget(TEXT("AmmoRadial"))) : nullptr;

    //「WidgetTree && WidgetTree->RootWidget && DesignerRadial」が成立するとき、m_pRadialWidgetを更新します。
    if (WidgetTree && WidgetTree->RootWidget && DesignerRadial)
    {
        m_pRadialWidget = DesignerRadial;
        m_pClipText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ClipAmmoText")));
        m_pReserveText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ReserveAmmoText")));
        return;
    }

    //未設定または旧形式のBlueprintでは、必要要素を備えたC++製HUDへ切り替えます。
    UCanvasPanel* rootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("AmmoRoot"));
    WidgetTree->RootWidget = rootPanel;

    //ammoOverlayは、WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Am…から取得した参照を後続の呼び出しで使います。
    UOverlay* ammoOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("AmmoOverlay"));

    //overlaySlotは、rootPanel->AddChildToCanvas(ammoOverlay)から取得した参照を後続の呼び出しで使います。
    UCanvasPanelSlot* overlaySlot = rootPanel->AddChildToCanvas(ammoOverlay);
    overlaySlot->SetAnchors(FAnchors(1.0f, 1.0f));
    overlaySlot->SetAlignment(FVector2D(1.0f, 1.0f));
    overlaySlot->SetPosition(FVector2D(-48.0f, -42.0f));
    overlaySlot->SetSize(FVector2D(190.0f, 190.0f));

     //使用するクラス情報を取得します。
    m_pRadialWidget =  WidgetTree->ConstructWidget<UAmmoRadialWidget>(UAmmoRadialWidget::StaticClass(), TEXT("AmmoRadial"));
    //radialSlotは、ammoOverlay->AddChildToOverlay(m_pRadialWidget)から取得した参照を後続の呼び出しで使います。
    UOverlaySlot* radialSlot = ammoOverlay->AddChildToOverlay(m_pRadialWidget);
    radialSlot->SetHorizontalAlignment(HAlign_Fill);
    radialSlot->SetVerticalAlignment(VAlign_Fill);

    //ammoNumberBoxは、WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Am…から取得した参照を後続の呼び出しで使います。
    USizeBox* ammoNumberBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AmmoNumberBox"));
    ammoNumberBox->SetWidthOverride(106.0f);
    ammoNumberBox->SetHeightOverride(70.0f);

    //numberBoxSlotは、ammoOverlay->AddChildToOverlay(ammoNumberBox)から取得した参照を後続の呼び出しで使います。
    UOverlaySlot* numberBoxSlot = ammoOverlay->AddChildToOverlay(ammoNumberBox);
    numberBoxSlot->SetHorizontalAlignment(HAlign_Center);
    numberBoxSlot->SetVerticalAlignment(VAlign_Center);

     //使用するクラス情報を取得します。
    UCanvasPanel* ammoNumberCanvas =  WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("AmmoNumberCanvas"));
    ammoNumberBox->SetContent(ammoNumberCanvas);

    m_pClipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ClipAmmoText"));
    m_pClipText->SetJustification(ETextJustify::Center);
    m_pClipText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    m_pClipText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 32, TEXT("Bold")));

    //clipSlotは、ammoNumberCanvas->AddChildToCanvas(m_pClipText)から取得した参照を後続の呼び出しで使います。
    UCanvasPanelSlot* clipSlot = ammoNumberCanvas->AddChildToCanvas(m_pClipText);
    clipSlot->SetPosition(FVector2D(3.0f, 0.0f));
    clipSlot->SetSize(FVector2D(52.0f, 42.0f));

    //separatorTextは、WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT…から取得した参照を後続の呼び出しで使います。
    UTextBlock* separatorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmmoSeparatorText"));
    separatorText->SetText(FText::FromString(TEXT("/")));
    separatorText->SetJustification(ETextJustify::Center);
    separatorText->SetColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.72f, 0.74f, 1.0f)));
    separatorText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 18, TEXT("Bold")));

    //separatorSlotは、ammoNumberCanvas->AddChildToCanvas(separatorText)から取得した参照を後続の呼び出しで使います。
    UCanvasPanelSlot* separatorSlot = ammoNumberCanvas->AddChildToCanvas(separatorText);
    separatorSlot->SetPosition(FVector2D(43.0f, 21.0f));
    separatorSlot->SetSize(FVector2D(24.0f, 28.0f));

    m_pReserveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReserveAmmoText"));
    m_pReserveText->SetJustification(ETextJustify::Center);
    m_pReserveText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.12f, 0.1f, 1.0f)));
    m_pReserveText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 15, TEXT("Bold")));

    //reserveSlotは、ammoNumberCanvas->AddChildToCanvas(m_pReserveText)から取得した参照を後続の呼び出しで使います。
    UCanvasPanelSlot* reserveSlot = ammoNumberCanvas->AddChildToCanvas(m_pReserveText);
    reserveSlot->SetPosition(FVector2D(57.0f, 37.0f));
    reserveSlot->SetSize(FVector2D(48.0f, 25.0f));
}

//武器を設定します。
void UAmmoHUDWidget::SetWeapon(AGunWeapon* _gunWeapon)
{
    //「m_pWeapon.Get() == _gunWeapon」が成立するとき、UpdateAmmoを呼び出します。
    if (m_pWeapon.Get() == _gunWeapon)
    {
        UpdateAmmo();
        return;
    }

    UnbindWeapon();
    m_pWeapon = _gunWeapon;

    //「m_pWeapon.IsValid()」が成立するとき、AddUniqueDynamicを呼び出します。
    if (m_pWeapon.IsValid())
    {
        m_pWeapon->OnReloadFinished.AddUniqueDynamic(this, &UAmmoHUDWidget::UpdateAmmo);
        SetVisibility(ESlateVisibility::HitTestInvisible);
        UpdateAmmo();
    }
    else
    {
        SetVisibility(ESlateVisibility::Collapsed);
    }
}

//Widget破棄時にDelegateを解除し、無効な参照を残さないようにします。
void UAmmoHUDWidget::NativeDestruct()
{
    UnbindWeapon();
    //Widget破棄時にDelegateを解除し、無効な参照を残さないようにします。
    Super::NativeDestruct();
}

//弾薬を更新します。
void UAmmoHUDWidget::UpdateAmmo()
{
    //「!m_pWeapon.IsValid()」が成立するとき、GetCurrentAmmoを呼び出します。
    if (!m_pWeapon.IsValid()) { return; }

    //現在の弾薬を返します。
    const int32 clipAmmo = m_pWeapon->GetCurrentAmmo();

    //maxClipAmmoは、FMath::Max(1, m_pWeapon->GetMaxClipAmmo())から算出した数値を後続の判定または計算に使います。
    const int32 maxClipAmmo = FMath::Max(1, m_pWeapon->GetMaxClipAmmo());

    //reserveAmmoは、m_pWeapon->GetTotalAmmo()から算出した数値を後続の判定または計算に使います。
    const int32 reserveAmmo = m_pWeapon->GetTotalAmmo();

    //maxReserveAmmoは、FMath::Max(1, m_pWeapon->GetTotalMaxAmmo())から算出した数値を後続の判定または計算に使います。
    const int32 maxReserveAmmo = FMath::Max(1, m_pWeapon->GetTotalMaxAmmo());

    //「m_pRadialWidget」が成立するとき、SetAmmoPercentを呼び出します。
    if (m_pRadialWidget)
    {
        m_pRadialWidget->SetAmmoPercent(static_cast<float>(clipAmmo) / static_cast<float>(maxClipAmmo),
                                        static_cast<float>(reserveAmmo) / static_cast<float>(maxReserveAmmo));
    }
    //「m_pClipText」が成立するとき、SetTextを呼び出します。
    if (m_pClipText)
    {
        m_pClipText->SetText(FText::AsNumber(clipAmmo));
    }
    //「m_pReserveText」が成立するとき、SetTextを呼び出します。
    if (m_pReserveText)
    {
        m_pReserveText->SetText(FText::AsNumber(reserveAmmo));
    }
}

//Weaponの通知先を解除します。
void UAmmoHUDWidget::UnbindWeapon()
{
    //「m_pWeapon.IsValid()」が成立するとき、RemoveDynamicを呼び出します。
    if (m_pWeapon.IsValid())
    {
        m_pWeapon->OnReloadFinished.RemoveDynamic(this, &UAmmoHUDWidget::UpdateAmmo);
    }
    m_pWeapon.Reset();
}
