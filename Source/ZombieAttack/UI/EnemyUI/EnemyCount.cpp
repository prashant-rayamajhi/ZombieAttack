#include "EnemyCount.h"

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
//MakeHudFontは、MakeHudFontの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
FSlateFontInfo MakeHudFont(int32 Size)
{
    //Mission HUDは可読性を最優先し、端末依存や欠損グリフのないEngine標準Fontを使います。
    //画面装飾は色・余白・アニメーションでゲームの雰囲気を維持します。
    return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size);
}

//AddTextは、名前が示す対象を既存の状態または一覧へ追加します。
UTextBlock* AddText(UWidgetTree* WidgetTree, UVerticalBox* Parent, const TCHAR* Name, const FText& Text, int32 FontSize, const FLinearColor& Color)
{
    //Labelは、WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name)から取得した参照を後続の呼び出しで使います。
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    Label->SetText(Text);
    Label->SetFont(MakeHudFont(FontSize));
    Label->SetColorAndOpacity(FSlateColor(Color));
    Label->SetJustification(ETextJustify::Center);
    Parent->AddChildToVerticalBox(Label);
    //Labelは、UI部品を構築または更新する呼び出しで参照するために使います。
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

    //DesignerEnemyCountは、WidgetTree ? Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("EnemyCountVa…から取得した参照を後続の呼び出しで使います。
    UTextBlock* DesignerEnemyCount = WidgetTree ? Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("EnemyCountValue"))) : nullptr;
    //「!DesignerEnemyCount && WidgetTree」が成立するとき、FindWidgetを呼び出します。
    if (!DesignerEnemyCount && WidgetTree)
    {
        DesignerEnemyCount = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("m_enemyCountText")));
    }

    //「WidgetTree && WidgetTree->RootWidget && DesignerEnemyCount」が成立するとき、m_enemyCountTextを更新します。
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
}

//ミッションHUDを作成します。
void UEnemyCount::BuildMissionHUD()
{
    //「!WidgetTree」が成立するとき、StaticClassを呼び出します。
    if (!WidgetTree) { return; }

    //Rootは、WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), …から取得した参照を後続の呼び出しで使います。
    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MissionHudRoot"));
    WidgetTree->RootWidget = Root;

    m_counterPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HostileCounterPanel"));
    m_counterPanel->SetBrushColor(FLinearColor(0.018f, 0.022f, 0.027f, 0.92f));
    m_counterPanel->SetPadding(FMargin(18.0f, 10.0f));
    Root->AddChild(m_counterPanel);
    //「UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(m_counterPanel->Slot)」が成立するとき、SetAnchorsを呼び出します。
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
    //「UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(m_missionPanel->Slot)」が成立するとき、SetAnchorsを呼び出します。
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
void UEnemyCount::UpdateEnemyCountUI(int32 CurrentCount)
{
    CurrentCount = FMath::Max(0, CurrentCount);
    //「m_enemyCountText」が成立するとき、SetTextを呼び出します。
    if (m_enemyCountText)
    {
        m_enemyCountText->SetText(FText::AsNumber(CurrentCount));
        //「m_lastEnemyCount != INDEX_NONE && CurrentCount < m_lastEnemyCount」が成立するとき、m_counterPulseRemainingを更新します。
        if (m_lastEnemyCount != INDEX_NONE && CurrentCount < m_lastEnemyCount)
        {
            m_counterPulseRemaining = 0.35f;
            m_enemyCountText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.12f, 0.06f, 1.0f)));
        }
    }
    m_lastEnemyCount = CurrentCount;
}

//MissionObjectiveを画面へ表示します。
void UEnemyCount::ShowMissionObjective(int32 InitialEnemyCount)
{
    //「m_bObjectiveShown」が成立するとき、m_bObjectiveShownを更新します。
    if (m_bObjectiveShown) { return; }

    m_bObjectiveShown = true;
    SetVisibility(ESlateVisibility::HitTestInvisible);

    //ミッション告知を先に見せ、その後に敵数パネルを左から表示する
    if (m_counterPanel)
    {
        m_counterPanel->SetVisibility(ESlateVisibility::Collapsed);
        m_counterPanel->SetRenderOpacity(0.0f);
        m_counterPanel->SetRenderTranslation(FVector2D(-140.0f, 0.0f));
        m_counterRevealDelay = 0.85f;
        m_counterRevealElapsed = 0.0f;
        m_bCounterRevealPending = true;
    }

    ShowCenterMessage(FText::FromString(TEXT("NEW MISSION")), FText::FromString(TEXT("ELIMINATE ALL HOSTILES")), 5.0f);
    //「m_missionCountText」が成立するとき、SetTextを呼び出します。
    if (m_missionCountText)
    {
        m_missionCountText->SetText(
            FText::Format(FText::FromString(TEXT("{0} TARGETS DETECTED")), FText::AsNumber(FMath::Max(0, InitialEnemyCount))));
    }
}

//ゴール準備完了を画面へ表示します。
void UEnemyCount::ShowGoalReady()
{
    ShowCenterMessage(FText::FromString(TEXT("OBJECTIVE COMPLETE")), FText::FromString(TEXT("EVACUATION POINT UNLOCKED")), 4.5f);
    //「m_missionCountText」が成立するとき、SetTextを呼び出します。
    if (m_missionCountText)
    {
        m_missionCountText->SetText(FText::FromString(TEXT("FOLLOW THE GREEN BEACON")));
    }
}

//CenterMessageを画面へ表示します。
void UEnemyCount::ShowCenterMessage(const FText& Header, const FText& Message, float Duration)
{
    //「!m_missionPanel」が成立するとき、SetTextを呼び出します。
    if (!m_missionPanel) { return; }

    m_missionHeaderText->SetText(Header);
    m_missionMessageText->SetText(Message);
    m_missionPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
    m_missionPanel->SetRenderOpacity(0.0f);
    m_missionPanel->SetRenderScale(FVector2D(0.92f, 0.92f));
    m_missionDuration = FMath::Max(0.1f, Duration);
    m_missionTimeRemaining = m_missionDuration;
}

//Widgetのアニメーションと表示値をフレームごとに更新します。
void UEnemyCount::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    //Widgetのアニメーションと表示値をフレームごとに更新します。
    Super::NativeTick(MyGeometry, InDeltaTime);

    //「m_bCounterRevealPending && m_counterPanel」が成立するとき、Maxを呼び出します。
    if (m_bCounterRevealPending && m_counterPanel)
    {
        m_counterRevealDelay = FMath::Max(0.0f, m_counterRevealDelay - InDeltaTime);
        //「m_counterRevealDelay <= 0.0f」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
        if (m_counterRevealDelay <= 0.0f)
        {
            constexpr float revealDuration = 0.42f;
            m_counterRevealElapsed += InDeltaTime;
            //透明度を保持します。
            const float alpha = FMath::Clamp(m_counterRevealElapsed / revealDuration, 0.0f, 1.0f);
            //視点の動きが急に変わらないよう加速率を整えます。
            const float easedAlpha = 1.0f - FMath::Pow(1.0f - alpha, 3.0f);

            m_counterPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
            m_counterPanel->SetRenderOpacity(easedAlpha);
            m_counterPanel->SetRenderTranslation(FVector2D(-140.0f * (1.0f - easedAlpha), 0.0f));

            //「alpha >= 1.0f」が成立するとき、m_bCounterRevealPendingを更新します。
            if (alpha >= 1.0f)
            {
                m_bCounterRevealPending = false;
            }
        }
    }

    //「m_counterPulseRemaining > 0.0f && m_enemyCountText」が成立するとき、Maxを呼び出します。
    if (m_counterPulseRemaining > 0.0f && m_enemyCountText)
    {
        m_counterPulseRemaining = FMath::Max(0.0f, m_counterPulseRemaining - InDeltaTime);
        //透明度を保持します。
        const float Alpha = m_counterPulseRemaining / 0.35f;
        //Scaleは、1.0f + FMath::Sin(Alpha * PI) * 0.16fから算出した数値を後続の判定または計算に使います。
        const float Scale = 1.0f + FMath::Sin(Alpha * PI) * 0.16f;
        m_enemyCountText->SetRenderScale(FVector2D(Scale, Scale));
        //「m_counterPulseRemaining <= 0.0f」が成立するとき、SetColorAndOpacityを呼び出します。
        if (m_counterPulseRemaining <= 0.0f)
        {
            m_enemyCountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        }
    }

    //「m_missionTimeRemaining > 0.0f && m_missionPanel」が成立するとき、Maxを呼び出します。
    if (m_missionTimeRemaining > 0.0f && m_missionPanel)
    {
        m_missionTimeRemaining = FMath::Max(0.0f, m_missionTimeRemaining - InDeltaTime);
        //Elapsedは、m_missionDuration - m_missionTimeRemainingから算出した数値を後続の判定または計算に使います。
        const float Elapsed = m_missionDuration - m_missionTimeRemaining;
        //FadeInは、FMath::Clamp(Elapsed / 0.35f, 0.0f, 1.0f)から算出した数値を後続の判定または計算に使います。
        const float FadeIn = FMath::Clamp(Elapsed / 0.35f, 0.0f, 1.0f);
        //FadeOutは、FMath::Clamp(m_missionTimeRemaining / 0.6f, 0.0f, 1.0f)から算出した数値を後続の判定または計算に使います。
        const float FadeOut = FMath::Clamp(m_missionTimeRemaining / 0.6f, 0.0f, 1.0f);
        //Opacityは、FMath::Min(FadeIn, FadeOut)から算出した数値を後続の判定または計算に使います。
        const float Opacity = FMath::Min(FadeIn, FadeOut);
        m_missionPanel->SetRenderOpacity(Opacity);
        m_missionPanel->SetRenderScale(FVector2D(FMath::Lerp(0.92f, 1.0f, FadeIn)));
        //「m_missionTimeRemaining <= 0.0f」が成立するとき、SetVisibilityを呼び出します。
        if (m_missionTimeRemaining <= 0.0f)
        {
            m_missionPanel->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}
