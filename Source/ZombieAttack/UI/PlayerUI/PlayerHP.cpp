#include "PlayerHP.h"
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
    //textは、UI部品を構築または更新する呼び出しで参照するために使います。
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

    //DesignerHealthBarは、WidgetTree ? Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("HealthBar"…から取得した参照を後続の呼び出しで使います。
    UProgressBar* DesignerHealthBar = WidgetTree ? Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("HealthBar"))) : nullptr;
    //「!DesignerHealthBar && WidgetTree」が成立するとき、FindWidgetを呼び出します。
    if (!DesignerHealthBar && WidgetTree)
    {
        DesignerHealthBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("m_healthBar")));
    }

    //「WidgetTree && WidgetTree->RootWidget && DesignerHealthBar」が成立するとき、m_healthBarを更新します。
    if (WidgetTree && WidgetTree->RootWidget && DesignerHealthBar)
    {
        m_healthBar = DesignerHealthBar;
        m_currentHealth = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("CurrentHealth")));
        //「!m_currentHealth」が成立するとき、FindWidgetを呼び出します。
        if (!m_currentHealth)
        {
            m_currentHealth = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("m_currentHealth")));
        }
        m_maxHealth = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("MaxHealth")));
        //「!m_maxHealth」が成立するとき、FindWidgetを呼び出します。
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
}

//画面左下に、廃墟の端末をイメージしたHPパネルを構築する
void UPlayerHP::BuildHealthHUD()
{
    //「!WidgetTree」が成立するとき、StaticClassを呼び出します。
    if (!WidgetTree) { return; }

    //rootは、WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), …から取得した参照を後続の呼び出しで使います。
    UCanvasPanel* root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HealthHudRoot"));
    WidgetTree->RootWidget = root;

    //HP低下量と直近の被弾を画面全体の薄い赤で伝えます。
    m_damageScreenOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DamageScreenOverlay"));
    m_damageScreenOverlay->SetBrushColor(FLinearColor(0.62f, 0.0f, 0.0f, 1.0f));
    m_damageScreenOverlay->SetRenderOpacity(0.0f);
    m_damageScreenOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
    root->AddChild(m_damageScreenOverlay);
    //「UCanvasPanelSlot* overlaySlot = Cast<UCanvasPanelSlot>(m_damageScreenOverlay->Slot)」が成立するとき、SetAnchorsを呼び出します。
    if (UCanvasPanelSlot* overlaySlot = Cast<UCanvasPanelSlot>(m_damageScreenOverlay->Slot))
    {
        overlaySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        overlaySlot->SetOffsets(FMargin(0.0f));
    }

    m_healthPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HealthPanel"));
    m_healthPanel->SetBrushColor(FLinearColor(0.015f, 0.018f, 0.020f, 0.94f));
    m_healthPanel->SetPadding(FMargin(18.0f, 11.0f));
    root->AddChild(m_healthPanel);
    //「UCanvasPanelSlot* slot = Cast<UCanvasPanelSlot>(m_healthPanel->Slot)」が成立するとき、SetAnchorsを呼び出します。
    if (UCanvasPanelSlot* slot = Cast<UCanvasPanelSlot>(m_healthPanel->Slot))
    {
        slot->SetAnchors(FAnchors(0.0f, 1.0f));
        slot->SetAlignment(FVector2D(0.0f, 1.0f));
        slot->SetPosition(FVector2D(32.0f, -34.0f));
        slot->SetSize(FVector2D(360.0f, 112.0f));
    }

    //stackは、WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), …から取得した参照を後続の呼び出しで使います。
    UVerticalBox* stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HealthStack"));
    m_healthPanel->SetContent(stack);

    UHorizontalBox* header =
        //使用するクラス情報を取得します。
        WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HealthHeader"));
    stack->AddChildToVerticalBox(header);
    AddHealthText(WidgetTree, header, TEXT("MedicalIcon"), TEXT("+"), 25, FLinearColor(0.16f, 0.95f, 0.32f, 1.0f));
    AddHealthText(WidgetTree, header, TEXT("VitalLabel"), TEXT("  HP"), 15, FLinearColor(0.82f, 0.86f, 0.87f, 1.0f));
    m_healthValue = AddHealthText(WidgetTree, header, TEXT("HealthValue"), TEXT("100 / 100"), 18, FLinearColor::White);
    //「UHorizontalBoxSlot* valueSlot = Cast<UHorizontalBoxSlot>(m_healthValue->Slot)」が成立するとき、SetHorizontalAlignmentを呼び出します。
    if (UHorizontalBoxSlot* valueSlot = Cast<UHorizontalBoxSlot>(m_healthValue->Slot))
    {
        valueSlot->SetHorizontalAlignment(HAlign_Right);
        valueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    m_healthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
    m_healthBar->SetPercent(1.0f);
    m_healthBar->SetFillColorAndOpacity(FLinearColor(0.08f, 0.84f, 0.25f, 1.0f));
    stack->AddChildToVerticalBox(m_healthBar);
    //「UVerticalBoxSlot* barSlot = Cast<UVerticalBoxSlot>(m_healthBar->Slot)」が成立するとき、SetPaddingを呼び出します。
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
    //「!m_owner.IsValid() || !m_healthBar」が成立するとき、GetHPを呼び出します。
    if (!m_owner.IsValid() || !m_healthBar) { return; }

    //体力を返します。
    const float CurrentHP = m_owner->GetHP();
    //最大体力を返します。
    const float MaxHP = m_owner->GetMaxHP();
    //healthRatioは、MaxHP > 0.f ? FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f) : 0.0fから算出した数値を後続の判定または計算に使います。
    const float healthRatio = MaxHP > 0.f ? FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f) : 0.0f;
    m_targetHealthRatio = healthRatio;

    //「m_healthValue」が成立するとき、SetTextを呼び出します。
    if (m_healthValue)
    {
        //現在値と最大値を体力表示へ反映します。
        m_healthValue->SetText(FText::Format(FText::FromString(TEXT("{0} / {1}")), FText::AsNumber(FMath::RoundToInt(CurrentHP)),
                                             FText::AsNumber(FMath::RoundToInt(MaxHP))));
    }
    //「healthRatio + KINDA_SMALL_NUMBER < m_lastHealthRatio」が成立するとき、m_damagePulseRemainingを更新します。
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

    //「m_currentHealth」が成立するとき、SetTextを呼び出します。
    if (m_currentHealth)
    {
        m_currentHealth->SetText(FText::AsNumber(FMath::RoundToInt(CurrentHP)));
    }
    //「m_maxHealth」が成立するとき、SetTextを呼び出します。
    if (m_maxHealth)
    {
        m_maxHealth->SetText(FText::AsNumber(FMath::RoundToInt(MaxHP)));
    }
}

//Widgetのアニメーションと表示値をフレームごとに更新します。
void UPlayerHP::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    //Widgetのアニメーションと表示値をフレームごとに更新します。
    Super::NativeTick(MyGeometry, InDeltaTime);

    //HP量は瞬間移動させず、短い補間で増減方向を読み取りやすくします。
    m_displayedHealthRatio = FMath::FInterpTo(m_displayedHealthRatio, m_targetHealthRatio, InDeltaTime, m_bHealingPulse ? 5.5f : 8.5f);
    //「m_healthBar」が成立するとき、SetPercentを呼び出します。
    if (m_healthBar)
    {
        m_healthBar->SetPercent(m_displayedHealthRatio);
        //体力色を保持します。
        const FLinearColor healthColor = m_displayedHealthRatio > 0.55f ? FLinearColor(0.08f, 0.84f, 0.25f, 1.0f)
                                                                        : (m_displayedHealthRatio > 0.25f ? FLinearColor(0.96f, 0.55f, 0.05f, 1.0f)
                                                                                                          : FLinearColor(0.95f, 0.05f, 0.03f, 1.0f));
        m_healthBar->SetFillColorAndOpacity(healthColor);
    }

    //「m_damageScreenOverlay」が成立するとき、Squareを呼び出します。
    if (m_damageScreenOverlay)
    {
        //低HPほど常時うっすら赤くし、被弾直後だけ強いフラッシュを重ねます。
        const float criticalTint = FMath::Square(1.0f - m_displayedHealthRatio) * 0.17f;
        //hitFlashは、0.0fから算出した数値を後続の判定または計算に使います。
        float hitFlash = 0.0f;
        //「!m_bHealingPulse && m_damagePulseRemaining > 0.0f」が成立するとき、Clampを呼び出します。
        if (!m_bHealingPulse && m_damagePulseRemaining > 0.0f)
        {
            //normalizedは、FMath::Clamp(m_damagePulseRemaining / 0.42f, 0.0f, 1.0f)から算出した数値を後続の判定または計算に使います。
            const float normalized = FMath::Clamp(m_damagePulseRemaining / 0.42f, 0.0f, 1.0f);
            hitFlash = normalized * normalized * 0.24f;
        }
        m_damageScreenOverlay->SetRenderOpacity(FMath::Clamp(criticalTint + hitFlash, 0.0f, 0.34f));
    }

    //「m_damagePulseRemaining <= 0.0f || !m_healthPanel」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (m_damagePulseRemaining <= 0.0f || !m_healthPanel) { return; }

    //pulseDurationは、m_bHealingPulse ? 0.52f : 0.42fから算出した数値を後続の判定または計算に使います。
    const float pulseDuration = m_bHealingPulse ? 0.52f : 0.42f;
    m_damagePulseRemaining = FMath::Max(0.0f, m_damagePulseRemaining - InDeltaTime);
    //透明度を保持します。
    const float alpha = m_damagePulseRemaining / pulseDuration;
    //pulseは、FMath::Sin(alpha * PI)から算出した数値を後続の判定または計算に使います。
    const float pulse = FMath::Sin(alpha * PI);
    //scaleAmountは、m_bHealingPulse ? 0.065f : 0.045fから算出した数値を後続の判定または計算に使います。
    const float scaleAmount = m_bHealingPulse ? 0.065f : 0.045f;
    m_healthPanel->SetRenderScale(FVector2D(1.0f + pulse * scaleAmount));
    //pulseColorは、m_bHealingPulse ? FLinearColor(0.01f, 0.22f, 0.045f, 0.98f) : FLinearCo…から構築した結果を後続の処理へ渡すために使います。
    const FLinearColor pulseColor = m_bHealingPulse ? FLinearColor(0.01f, 0.22f, 0.045f, 0.98f) : FLinearColor(0.24f, 0.01f, 0.01f, 0.98f);
    m_healthPanel->SetBrushColor(FMath::Lerp(FLinearColor(0.015f, 0.018f, 0.020f, 0.94f), pulseColor, pulse * 0.72f));
    //「m_damagePulseRemaining <= 0.0f」が成立するとき、SetRenderScaleを呼び出します。
    if (m_damagePulseRemaining <= 0.0f)
    {
        m_healthPanel->SetRenderScale(FVector2D(1.0f));
        m_healthPanel->SetBrushColor(FLinearColor(0.015f, 0.018f, 0.020f, 0.94f));
    }
}
//所有者を設定します。
void UPlayerHP::SetOwner(APlayerChara* Player) { m_owner = Player; }
