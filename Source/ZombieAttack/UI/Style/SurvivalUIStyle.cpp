#include "SurvivalUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanelSlot.h"
#include "Styling/CoreStyle.h"

//BP側に旧配色が保存されていても、インゲームの基本部品を同じ見た目に揃える。
void SurvivalUI::Apply(UWidgetTree* _tree)
{
    if (!_tree) { return; }
    _tree->ForEachWidget([](UWidget* _widget)
    {
        const FName name = _widget->GetFName();
        if (UTextBlock* text = Cast<UTextBlock>(_widget))
        {
            text->SetColorAndOpacity(FSlateColor(Text));
            text->SetShadowOffset(FVector2D(1, 1));
            text->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.8f));
            if (name == TEXT("ThreatLabel")) { text->SetVisibility(ESlateVisibility::Collapsed); }
            if (name == TEXT("RemainingLabel"))
            {
                text->SetText(FText::AsCultureInvariant(TEXT("INFECTED REMAINING")));
                text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 13));
                text->SetColorAndOpacity(FSlateColor(Muted));
            }
            if (name == TEXT("EnemyCountValue")) { text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 28)); }
            if (name == TEXT("HealthValue")) { text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 17)); }
            if (name == TEXT("ReserveAmmoText")) { text->SetColorAndOpacity(FSlateColor(Accent)); }
        }
        if (UBorder* border = Cast<UBorder>(_widget))
        {
            if (name == TEXT("HostileCounterPanel") || name == TEXT("HealthPanel") || name == TEXT("MissionAnnouncementPanel"))
            {
                FSlateBrush brush;
                brush.TintColor = Panel;
                brush.DrawAs = ESlateBrushDrawType::Box;
                border->SetBrush(brush);
                border->SetPadding(FMargin(16, 10));
            }
            if (UCanvasPanelSlot* slot = Cast<UCanvasPanelSlot>(border->Slot))
            {
                if (name == TEXT("HostileCounterPanel")) { slot->SetSize(FVector2D(270, 82)); }
                if (name == TEXT("HealthPanel")) { slot->SetSize(FVector2D(300, 76)); }
            }
        }
        if (UProgressBar* bar = Cast<UProgressBar>(_widget))
        {
            FProgressBarStyle style = bar->GetWidgetStyle();
            style.BackgroundImage = *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
            style.BackgroundImage.TintColor = FLinearColor(0.06f, 0.07f, 0.06f);
            style.FillImage = *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
            bar->SetWidgetStyle(style);
            bar->SetFillColorAndOpacity(Health);
        }
    });
}
