#include "AmmoHUDWidget.h"
#include "../Style/SurvivalUIStyle.h"

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
    UAmmoRadialWidget* DesignerRadial = WidgetTree ? Cast<UAmmoRadialWidget>(WidgetTree->FindWidget(TEXT("AmmoRadial"))) : nullptr;
    if (WidgetTree && WidgetTree->RootWidget && DesignerRadial)
    {
        m_pRadialWidget = DesignerRadial;
        m_pClipText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ClipAmmoText")));
        m_pReserveText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ReserveAmmoText")));
        SurvivalUI::Apply(WidgetTree);
        return;
    }

    //未設定または旧形式のBlueprintでは、必要要素を備えたC++製HUDへ切り替えます。
    UCanvasPanel* rootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("AmmoRoot"));
    WidgetTree->RootWidget = rootPanel;
    UOverlay* ammoOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("AmmoOverlay"));
    UCanvasPanelSlot* overlaySlot = rootPanel->AddChildToCanvas(ammoOverlay);
    overlaySlot->SetAnchors(FAnchors(1.0f, 1.0f));
    overlaySlot->SetAlignment(FVector2D(1.0f, 1.0f));
    overlaySlot->SetPosition(FVector2D(-48.0f, -42.0f));
    overlaySlot->SetSize(FVector2D(190.0f, 190.0f));

     //使用するクラス情報を取得します。
    m_pRadialWidget =  WidgetTree->ConstructWidget<UAmmoRadialWidget>(UAmmoRadialWidget::StaticClass(), TEXT("AmmoRadial"));
    UOverlaySlot* radialSlot = ammoOverlay->AddChildToOverlay(m_pRadialWidget);
    radialSlot->SetHorizontalAlignment(HAlign_Fill);
    radialSlot->SetVerticalAlignment(VAlign_Fill);
    USizeBox* ammoNumberBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AmmoNumberBox"));
    ammoNumberBox->SetWidthOverride(106.0f);
    ammoNumberBox->SetHeightOverride(70.0f);
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
    UCanvasPanelSlot* clipSlot = ammoNumberCanvas->AddChildToCanvas(m_pClipText);
    clipSlot->SetPosition(FVector2D(3.0f, 0.0f));
    clipSlot->SetSize(FVector2D(52.0f, 42.0f));
    UTextBlock* separatorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmmoSeparatorText"));
    separatorText->SetText(FText::FromString(TEXT("/")));
    separatorText->SetJustification(ETextJustify::Center);
    separatorText->SetColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.72f, 0.74f, 1.0f)));
    separatorText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 18, TEXT("Bold")));
    UCanvasPanelSlot* separatorSlot = ammoNumberCanvas->AddChildToCanvas(separatorText);
    separatorSlot->SetPosition(FVector2D(43.0f, 21.0f));
    separatorSlot->SetSize(FVector2D(24.0f, 28.0f));

    m_pReserveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReserveAmmoText"));
    m_pReserveText->SetJustification(ETextJustify::Center);
    m_pReserveText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.12f, 0.1f, 1.0f)));
    m_pReserveText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 15, TEXT("Bold")));
    UCanvasPanelSlot* reserveSlot = ammoNumberCanvas->AddChildToCanvas(m_pReserveText);
    reserveSlot->SetPosition(FVector2D(57.0f, 37.0f));
    reserveSlot->SetSize(FVector2D(48.0f, 25.0f));
    SurvivalUI::Apply(WidgetTree);
}

//武器を設定します。
void UAmmoHUDWidget::SetWeapon(AGunWeapon* _gunWeapon)
{
    if (m_pWeapon.Get() == _gunWeapon)
    {
        UpdateAmmo();
        return;
    }

    UnbindWeapon();
    m_pWeapon = _gunWeapon;
    if (m_pWeapon.IsValid())
    {
        m_pWeapon->m_onReloadFinished.AddUniqueDynamic(this, &UAmmoHUDWidget::UpdateAmmo);
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
    if (!m_pWeapon.IsValid()) { return; }

    //現在の弾薬を返します。
    const int32 clipAmmo = m_pWeapon->GetCurrentAmmo();
    const int32 maxClipAmmo = FMath::Max(1, m_pWeapon->GetMaxClipAmmo());
    const int32 reserveAmmo = m_pWeapon->GetTotalAmmo();
    const int32 maxReserveAmmo = FMath::Max(1, m_pWeapon->GetTotalMaxAmmo());
    if (m_pRadialWidget)
    {
        m_pRadialWidget->SetAmmoPercent(static_cast<float>(clipAmmo) / static_cast<float>(maxClipAmmo),
                                        static_cast<float>(reserveAmmo) / static_cast<float>(maxReserveAmmo));
    }
    if (m_pClipText)
    {
        m_pClipText->SetText(FText::AsNumber(clipAmmo));
        //数字の位置や大きさは変えず、残り四分の一でリロードの判断を促す。
        const bool lowAmmo = clipAmmo <= FMath::Max(1, maxClipAmmo / 4);
        const FLinearColor color = clipAmmo == 0 ? FLinearColor(1.0f, 0.18f, 0.12f) :
            (lowAmmo ? FLinearColor(1.0f, 0.65f, 0.18f) : FLinearColor::White);
        m_pClipText->SetColorAndOpacity(FSlateColor(color));
    }
    if (m_pReserveText)
    {
        m_pReserveText->SetText(FText::AsNumber(reserveAmmo));
    }
}

//Weaponの通知先を解除します。
void UAmmoHUDWidget::UnbindWeapon()
{
    if (m_pWeapon.IsValid())
    {
        m_pWeapon->m_onReloadFinished.RemoveDynamic(this, &UAmmoHUDWidget::UpdateAmmo);
    }
    m_pWeapon.Reset();
}
