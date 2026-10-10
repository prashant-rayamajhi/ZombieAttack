#include "PlayerHP.h"
#include "../Style/SurvivalUIStyle.h"
#include "../../Player/PlayerChara.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Styling/CoreStyle.h"

namespace
{
//AddHealthTextは、名前が示す対象を既存の状態または一覧へ追加します。
UTextBlock* AddHealthText(UWidgetTree* _tree, UPanelWidget* _parent, const FName _name, const FString& _text, int32 _size, const FLinearColor& _color)
{
    //体力表示に使う文字部品を作成します。
    UTextBlock* text = _tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), _name);
    text->SetText(FText::FromString(_text));
    text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), _size));
    text->SetColorAndOpacity(FSlateColor(_color));
    _parent->AddChild(text);
    return text;
}
//名前空間を閉じます。
//名前空間を閉じます。
}

//Widget生成時に子Widgetとゲーム側の通知を接続します。
void UPlayerHP::NativeOnInitialized()
{
    //Widget生成時に子Widgetとゲーム側の通知を接続します。
    Super::NativeOnInitialized();
    UProgressBar* DesignerHealthBar = WidgetTree ? Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("HealthBar"))) : nullptr;
    if (!DesignerHealthBar && WidgetTree)
    {
        DesignerHealthBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("m_healthBar")));
    }
    if (WidgetTree && WidgetTree->RootWidget && DesignerHealthBar)
    {
        m_healthBar = DesignerHealthBar;
        m_currentHealth = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("CurrentHealth")));
        if (!m_currentHealth)
        {
            m_currentHealth = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("m_currentHealth")));
        }
        m_maxHealth = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("MaxHealth")));
        if (!m_maxHealth)
        {
            m_maxHealth = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("m_maxHealth")));
        }
        m_healthValue = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("HealthValue")));
        m_healthPanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("HealthPanel")));
        m_damageScreenOverlay = Cast<UBorder>(WidgetTree->FindWidget(TEXT("DamageScreenOverlay")));
    }
    else
    {
        //体力HUDを作成します。
        BuildHealthHUD();
    }
    SurvivalUI::Apply(WidgetTree);
}

//画面左下に、廃墟の端末をイメージしたHPパネルを構築する
void UPlayerHP::BuildHealthHUD()
{
    if (!WidgetTree) { return; }
    UCanvasPanel* root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HealthHudRoot"));
    WidgetTree->RootWidget = root;

    //HP低下量と直近の被弾を画面全体の薄い赤で伝えます。
    m_damageScreenOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DamageScreenOverlay"));
    m_damageScreenOverlay->SetBrushColor(FLinearColor(0.62f, 0.0f, 0.0f, 1.0f));
    m_damageScreenOverlay->SetRenderOpacity(0.0f);
    m_damageScreenOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
    root->AddChild(m_damageScreenOverlay);
    if (UCanvasPanelSlot* overlaySlot = Cast<UCanvasPanelSlot>(m_damageScreenOverlay->Slot))
    {
        overlaySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        overlaySlot->SetOffsets(FMargin(0.0f));
    }

    m_healthPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HealthPanel"));
    m_healthPanel->SetBrushColor(FLinearColor(0.015f, 0.018f, 0.020f, 0.94f));
    m_healthPanel->SetPadding(FMargin(18.0f, 11.0f));
    root->AddChild(m_healthPanel);
    if (UCanvasPanelSlot* slot = Cast<UCanvasPanelSlot>(m_healthPanel->Slot))
    {
        slot->SetAnchors(FAnchors(0.0f, 1.0f));
        slot->SetAlignment(FVector2D(0.0f, 1.0f));
        slot->SetPosition(FVector2D(32.0f, -34.0f));
        slot->SetSize(FVector2D(360.0f, 112.0f));
    }
    UVerticalBox* stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HealthStack"));
    m_healthPanel->SetContent(stack);

    UHorizontalBox* header =
        //使用するクラス情報を取得します。
        WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HealthHeader"));
    stack->AddChildToVerticalBox(header);
    AddHealthText(WidgetTree, header, TEXT("MedicalIcon"), TEXT("+"), 25, FLinearColor(0.16f, 0.95f, 0.32f, 1.0f));
    AddHealthText(WidgetTree, header, TEXT("VitalLabel"), TEXT("  HP"), 15, FLinearColor(0.82f, 0.86f, 0.87f, 1.0f));
    m_healthValue = AddHealthText(WidgetTree, header, TEXT("HealthValue"), TEXT("100 / 100"), 18, FLinearColor::White);
    if (UHorizontalBoxSlot* valueSlot = Cast<UHorizontalBoxSlot>(m_healthValue->Slot))
    {
        valueSlot->SetHorizontalAlignment(HAlign_Right);
        valueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    m_healthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
    m_healthBar->SetPercent(1.0f);
    m_healthBar->SetFillColorAndOpacity(FLinearColor(0.08f, 0.84f, 0.25f, 1.0f));
    stack->AddChildToVerticalBox(m_healthBar);
    if (UVerticalBoxSlot* barSlot = Cast<UVerticalBoxSlot>(m_healthBar->Slot))
    {
        barSlot->SetPadding(FMargin(0.0f, 9.0f, 0.0f, 0.0f));
        barSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    m_currentHealth = nullptr;
    m_maxHealth = nullptr;
}

//体力UIを最新のゲーム状態へ更新します。
void UPlayerHP::UpdateHealthUI()
{
    if (!m_owner.IsValid() || !m_healthBar) { return; }

    //体力を返します。
    const float CurrentHP = m_owner->GetHP();
    //最大体力を返します。
    const float MaxHP = m_owner->GetMaxHP();
    const float healthRatio = MaxHP > 0.f ? FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f) : 0.0f;
    m_targetHealthRatio = healthRatio;
    if (m_healthValue)
    {
        //現在値と最大値を体力表示へ反映します。
        m_healthValue->SetText(FText::Format(FText::FromString(TEXT("{0} / {1}")), FText::AsNumber(FMath::RoundToInt(CurrentHP)),
                                             FText::AsNumber(FMath::RoundToInt(MaxHP))));
    }
    if (healthRatio + KINDA_SMALL_NUMBER < m_lastHealthRatio)
    {
        m_damagePulseRemaining = 0.42f;
        m_bHealingPulse = false;
    }
    else if (healthRatio > m_lastHealthRatio + KINDA_SMALL_NUMBER)
    {
        m_damagePulseRemaining = 0.52f;
        m_bHealingPulse = true;
    }
    m_lastHealthRatio = healthRatio;
    if (m_currentHealth)
    {
        m_currentHealth->SetText(FText::AsNumber(FMath::RoundToInt(CurrentHP)));
    }
    if (m_maxHealth)
    {
        m_maxHealth->SetText(FText::AsNumber(FMath::RoundToInt(MaxHP)));
    }
}

//Widgetのアニメーションと表示値をフレームごとに更新します。
void UPlayerHP::NativeTick(const FGeometry& _geometry, float _deltaTime)
{
    //Widgetのアニメーションと表示値をフレームごとに更新します。
    Super::NativeTick(_geometry, _deltaTime);

    //HP量は瞬間移動させず、短い補間で増減方向を読み取りやすくします。
    m_displayedHealthRatio = FMath::FInterpTo(m_displayedHealthRatio, m_targetHealthRatio, _deltaTime, m_bHealingPulse ? 5.5f : 8.5f);
    if (m_healthBar)
    {
        m_healthBar->SetPercent(m_displayedHealthRatio);
        //体力色を保持します。
        const FLinearColor healthColor = m_displayedHealthRatio > 0.55f ? SurvivalUI::Health
                                                                        : (m_displayedHealthRatio > 0.25f ? FLinearColor(0.96f, 0.55f, 0.05f, 1.0f)
                                                                                                          : FLinearColor(0.95f, 0.05f, 0.03f, 1.0f));
        m_healthBar->SetFillColorAndOpacity(healthColor);
    }
    if (m_damageScreenOverlay)
    {
        //低HPほど常時うっすら赤くし、被弾直後だけ強いフラッシュを重ねます。
        const float criticalTint = FMath::Square(1.0f - m_displayedHealthRatio) * 0.17f;
        float hitFlash = 0.0f;
        if (!m_bHealingPulse && m_damagePulseRemaining > 0.0f)
        {
            const float normalized = FMath::Clamp(m_damagePulseRemaining / 0.42f, 0.0f, 1.0f);
            hitFlash = normalized * normalized * 0.24f;
        }
        m_damageScreenOverlay->SetRenderOpacity(FMath::Clamp(criticalTint + hitFlash, 0.0f, 0.34f));
    }
    if (m_damagePulseRemaining <= 0.0f || !m_healthPanel) { return; }
    const float pulseDuration = m_bHealingPulse ? 0.52f : 0.42f;
    m_damagePulseRemaining = FMath::Max(0.0f, m_damagePulseRemaining - _deltaTime);
    //透明度を保持します。
    const float alpha = m_damagePulseRemaining / pulseDuration;
    const float pulse = FMath::Sin(alpha * PI);
    const float scaleAmount = 0.0f;
    m_healthPanel->SetRenderScale(FVector2D(1.0f + pulse * scaleAmount));
    const FLinearColor pulseColor = m_bHealingPulse ? FLinearColor(0.01f, 0.22f, 0.045f, 0.98f) : FLinearColor(0.24f, 0.01f, 0.01f, 0.98f);
    m_healthPanel->SetBrushColor(FMath::Lerp(FLinearColor(0.015f, 0.018f, 0.020f, 0.94f), pulseColor, pulse * 0.72f));
    if (m_damagePulseRemaining <= 0.0f)
    {
        m_healthPanel->SetRenderScale(FVector2D(1.0f));
        m_healthPanel->SetBrushColor(FLinearColor(0.015f, 0.018f, 0.020f, 0.94f));
    }
}
//所有者を設定します。
void UPlayerHP::SetOwner(APlayerChara* _pPlayer) { m_owner = _pPlayer; }
