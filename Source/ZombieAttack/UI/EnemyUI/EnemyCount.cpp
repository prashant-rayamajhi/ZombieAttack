#include "EnemyCount.h"
#include "../Style/SurvivalUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

namespace
{
FSlateFontInfo MakeHudFont(int32 Size)
{
    //Mission HUDは可読性を最優先し、端末依存や欠損グリフのないEngine標準Fontを使います。
    //画面装飾は色・余白・アニメーションでゲームの雰囲気を維持します。
    return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size);
}

//AddTextは、名前が示す対象を既存の状態または一覧へ追加します。
UTextBlock* AddText(UWidgetTree* WidgetTree, UVerticalBox* Parent, const TCHAR* Name, const FText& Text, int32 FontSize, const FLinearColor& Color)
{
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    Label->SetText(Text);
    Label->SetFont(MakeHudFont(FontSize));
    Label->SetColorAndOpacity(FSlateColor(Color));
    Label->SetJustification(ETextJustify::Center);
    Parent->AddChildToVerticalBox(Label);
    return Label;
}
//名前空間を閉じます。
//名前空間を閉じます。
}

//Widget生成時に子Widgetとゲーム側の通知を接続します。
void UEnemyCount::NativeOnInitialized()
{
    //Widget生成時に子Widgetとゲーム側の通知を接続します。
    Super::NativeOnInitialized();
    UTextBlock* DesignerEnemyCount = WidgetTree ? Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("EnemyCountValue"))) : nullptr;
    if (!DesignerEnemyCount && WidgetTree)
    {
        DesignerEnemyCount = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("m_enemyCountText")));
    }
    if (WidgetTree && WidgetTree->RootWidget && DesignerEnemyCount)
    {
        m_enemyCountText = DesignerEnemyCount;
        m_counterPanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("HostileCounterPanel")));
        m_missionPanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("MissionAnnouncementPanel")));
        m_missionHeaderText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("MissionHeader")));
        m_missionMessageText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("MissionMessage")));
        m_missionCountText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("MissionCount")));
    }
    else
    {
        //未設定または旧形式のBlueprintでは、必要要素を備えたC++製HUDへ切り替えます。
        BuildMissionHUD();
    }
    SurvivalUI::Apply(WidgetTree);
}

//ミッションHUDを作成します。
void UEnemyCount::BuildMissionHUD()
{
    if (!WidgetTree) { return; }
    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MissionHudRoot"));
    WidgetTree->RootWidget = Root;

    m_counterPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HostileCounterPanel"));
    m_counterPanel->SetBrushColor(FLinearColor(0.018f, 0.022f, 0.027f, 0.92f));
    m_counterPanel->SetPadding(FMargin(18.0f, 10.0f));
    Root->AddChild(m_counterPanel);
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(m_counterPanel->Slot))
    {
        CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
        CanvasSlot->SetPosition(FVector2D(32.0f, 32.0f));
        CanvasSlot->SetSize(FVector2D(330.0f, 108.0f));
    }

    UVerticalBox* CounterStack =
        //使用するクラス情報を取得します。
        WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CounterStack"));
    m_counterPanel->SetContent(CounterStack);
    AddText(WidgetTree, CounterStack, TEXT("ThreatLabel"), FText::FromString(TEXT("EXTERMINATION")), 13, FLinearColor(0.85f, 0.12f, 0.08f, 1.0f));
    AddText(WidgetTree, CounterStack, TEXT("RemainingLabel"), FText::FromString(TEXT("HOSTILES REMAINING")), 17,
            FLinearColor(0.78f, 0.82f, 0.84f, 1.0f));
    m_enemyCountText = AddText(WidgetTree, CounterStack, TEXT("EnemyCountValue"), FText::AsNumber(0), 32, FLinearColor::White);

    m_missionPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MissionAnnouncementPanel"));
    m_missionPanel->SetBrushColor(FLinearColor(0.008f, 0.012f, 0.018f, 0.93f));
    m_missionPanel->SetPadding(FMargin(34.0f, 18.0f));
    m_missionPanel->SetVisibility(ESlateVisibility::Collapsed);
    Root->AddChild(m_missionPanel);
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(m_missionPanel->Slot))
    {
        CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        CanvasSlot->SetPosition(FVector2D(0.0f, -130.0f));
        CanvasSlot->SetSize(FVector2D(720.0f, 180.0f));
    }

    UVerticalBox* MissionStack =
        //使用するクラス情報を取得します。
        WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MissionStack"));
    m_missionPanel->SetContent(MissionStack);
    m_missionHeaderText = AddText(WidgetTree, MissionStack, TEXT("MissionHeader"), FText::GetEmpty(), 15, FLinearColor(0.85f, 0.12f, 0.08f, 1.0f));
    m_missionMessageText = AddText(WidgetTree, MissionStack, TEXT("MissionMessage"), FText::GetEmpty(), 30, FLinearColor::White);
    m_missionCountText = AddText(WidgetTree, MissionStack, TEXT("MissionCount"), FText::GetEmpty(), 20, FLinearColor(0.72f, 0.76f, 0.78f, 1.0f));
}

//敵数UIを最新のゲーム状態へ更新します。
void UEnemyCount::UpdateEnemyCountUI(int32 _enemyCount)
{
    _enemyCount = FMath::Max(0, _enemyCount);
    if (m_enemyCountText)
    {
        m_enemyCountText->SetText(FText::AsNumber(_enemyCount));
        if (m_lastEnemyCount != INDEX_NONE && _enemyCount < m_lastEnemyCount)
        {
            m_counterPulseRemaining = 0.35f;
            m_enemyCountText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.12f, 0.06f, 1.0f)));
        }
    }
    m_lastEnemyCount = _enemyCount;
}

//MissionObjectiveを画面へ表示します。
void UEnemyCount::ShowMissionObjective(int32 _initialEnemyCount)
{
    if (m_bObjectiveShown) { return; }

    m_bObjectiveShown = true;
    SetVisibility(ESlateVisibility::HitTestInvisible);

    //ミッション告知を先に見せ、その後に敵数パネルを同じ位置でフェード表示する。
    if (m_counterPanel)
    {
        m_counterPanel->SetVisibility(ESlateVisibility::Collapsed);
        m_counterPanel->SetRenderOpacity(0.0f);
        m_counterPanel->SetRenderTranslation(FVector2D::ZeroVector);
        m_counterRevealDelay = 0.85f;
        m_counterRevealElapsed = 0.0f;
        m_bCounterRevealPending = true;
    }

    ShowCenterMessage(FText::FromString(TEXT("NEW MISSION")), FText::FromString(TEXT("ELIMINATE ALL HOSTILES")), 5.0f);
    if (m_missionCountText)
    {
        m_missionCountText->SetText(
            FText::Format(FText::FromString(TEXT("{0} TARGETS DETECTED")), FText::AsNumber(FMath::Max(0, _initialEnemyCount))));
    }
}

//ゴール準備完了を画面へ表示します。
void UEnemyCount::ShowGoalReady()
{
    ShowCenterMessage(FText::FromString(TEXT("OBJECTIVE COMPLETE")), FText::FromString(TEXT("EVACUATION POINT UNLOCKED")), 4.5f);
    if (m_missionCountText)
    {
        m_missionCountText->SetText(FText::FromString(TEXT("FOLLOW THE GREEN BEACON")));
    }
}

//CenterMessageを画面へ表示します。
void UEnemyCount::ShowCenterMessage(const FText& _header, const FText& _message, float _duration)
{
    if (!m_missionPanel) { return; }

    m_missionHeaderText->SetText(_header);
    m_missionMessageText->SetText(_message);
    m_missionPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
    m_missionPanel->SetRenderOpacity(0.0f);
    m_missionPanel->SetRenderScale(FVector2D(0.92f, 0.92f));
    m_missionDuration = FMath::Max(0.1f, _duration);
    m_missionTimeRemaining = m_missionDuration;
}

//Widgetのアニメーションと表示値をフレームごとに更新します。
void UEnemyCount::NativeTick(const FGeometry& _geometry, float _deltaTime)
{
    //Widgetのアニメーションと表示値をフレームごとに更新します。
    Super::NativeTick(_geometry, _deltaTime);
    if (m_bCounterRevealPending && m_counterPanel)
    {
        m_counterRevealDelay = FMath::Max(0.0f, m_counterRevealDelay - _deltaTime);
        if (m_counterRevealDelay <= 0.0f)
        {
            constexpr float revealDuration = 0.42f;
            m_counterRevealElapsed += _deltaTime;
            //透明度を保持します。
            const float alpha = FMath::Clamp(m_counterRevealElapsed / revealDuration, 0.0f, 1.0f);
            //視点の動きが急に変わらないよう加速率を整えます。
            const float easedAlpha = 1.0f - FMath::Pow(1.0f - alpha, 3.0f);

            m_counterPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
            m_counterPanel->SetRenderOpacity(easedAlpha);
            if (alpha >= 1.0f)
            {
                m_bCounterRevealPending = false;
            }
        }
    }
    if (m_counterPulseRemaining > 0.0f && m_enemyCountText)
    {
        m_counterPulseRemaining = FMath::Max(0.0f, m_counterPulseRemaining - _deltaTime);
        //透明度を保持します。
        const float Alpha = m_counterPulseRemaining / 0.35f;
        const float Scale = 1.0f + FMath::Sin(Alpha * PI) * 0.035f;
        m_enemyCountText->SetRenderScale(FVector2D(Scale, Scale));
        if (m_counterPulseRemaining <= 0.0f)
        {
            m_enemyCountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        }
    }
    if (m_missionTimeRemaining > 0.0f && m_missionPanel)
    {
        m_missionTimeRemaining = FMath::Max(0.0f, m_missionTimeRemaining - _deltaTime);
        const float Elapsed = m_missionDuration - m_missionTimeRemaining;
        const float FadeIn = FMath::Clamp(Elapsed / 0.35f, 0.0f, 1.0f);
        const float FadeOut = FMath::Clamp(m_missionTimeRemaining / 0.6f, 0.0f, 1.0f);
        const float Opacity = FMath::Min(FadeIn, FadeOut);
        m_missionPanel->SetRenderOpacity(Opacity);
        m_missionPanel->SetRenderScale(FVector2D(1.0f));
        if (m_missionTimeRemaining <= 0.0f)
        {
            m_missionPanel->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}
