#include "GameFlowScreenWidget.h"
#include "GameFlowBackdropWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Sound/SoundBase.h"

namespace
{
constexpr TCHAR GameplayLevelName[] = TEXT("GameLevel1");
constexpr TCHAR TitleLevelName[] = TEXT("GameStart");

//GetTitleは、呼び出し元が必要とする対象または計算結果を返します。
FText GetTitle(UGameFlowScreenWidget::EGameFlowScreen _screen)
{
    switch (_screen)
    {
    case UGameFlowScreenWidget::EGameFlowScreen::Clear: return FText::AsCultureInvariant(TEXT("MISSION COMPLETE"));
    case UGameFlowScreenWidget::EGameFlowScreen::GameOver: return FText::AsCultureInvariant(TEXT("YOU DIED"));
    default: return FText::AsCultureInvariant(TEXT("ZOMBIE ATTACK"));
    }
}

//GetSubtitleは、呼び出し元が必要とする対象または計算結果を返します。
FText GetSubtitle(UGameFlowScreenWidget::EGameFlowScreen _screen)
{
    switch (_screen)
    {
    case UGameFlowScreenWidget::EGameFlowScreen::Clear: return FText::AsCultureInvariant(TEXT("SURVIVED UNTIL DAWN"));
    case UGameFlowScreenWidget::EGameFlowScreen::GameOver: return FText::AsCultureInvariant(TEXT("THE INFECTION CLAIMED ANOTHER SURVIVOR"));
    default: return FText::AsCultureInvariant(TEXT("ESCAPE THE INFECTED FOREST"));
    }
}
//名前空間を閉じます。
//名前空間を閉じます。
}

//UGameFlowScreenWidgetが使用するComponentと初期パラメータを設定します。
UGameFlowScreenWidget::UGameFlowScreenWidget(const FObjectInitializer& _objectInitializer)
    : Super(_objectInitializer)
{
}

//Widget生成時に子Widgetとゲーム側の通知を接続します。
void UGameFlowScreenWidget::NativeOnInitialized()
{
    //Widget生成時に子Widgetとゲーム側の通知を接続します。
    Super::NativeOnInitialized();
    InitializeScreen();
}

//プレイヤー参照の初期化順に左右されず、表示時にボタンと背景を構築する
TSharedRef<SWidget> UGameFlowScreenWidget::RebuildWidget()
{
    if (!m_menu) { InitializeScreen(); }
    return Super::RebuildWidget();
}

//三画面の部品を揃え、表示する見出しと色だけをレベルに合わせる
void UGameFlowScreenWidget::InitializeScreen()
{
    if (m_menu) { return; }
    const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
    if (LevelName.Equals(TEXT("GameClear"), ESearchCase::IgnoreCase))
    {
        m_screen = EGameFlowScreen::Clear;
    }
    //現在のレベルがGameOver画面か確認します。
    else if (LevelName.Equals(TEXT("GameOver"), ESearchCase::IgnoreCase))
    {
        m_screen = EGameFlowScreen::GameOver;
    }
    else
    {
        m_screen = EGameFlowScreen::Start;
    }
    //旧Designerに保存された背景画像も表示せず、同じ部品で三画面を構成する
    BuildScreen(m_screen);
}

//Screenを作成します。
void UGameFlowScreenWidget::BuildScreen(EGameFlowScreen _screen)
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("GameFlowWidgetTree"));
    }
    UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
    WidgetTree->RootWidget = RootOverlay;
    UGameFlowBackdropWidget* Background = CreateWidget<UGameFlowBackdropWidget>(this);
    Background->SetScene(static_cast<int32>(_screen));
    Background->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* BackgroundSlot = RootOverlay->AddChildToOverlay(Background);
    BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
    BackgroundSlot->SetVerticalAlignment(VAlign_Fill);

    //背景を少し暗くして、どの解像度でも文字の可読性を保ちます。
    UBorder* Shade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundShade"));
    //文字の背後は専用グラデーションが受け持つため、人物まで暗幕で覆わない。
    FLinearColor shadeColor = m_backgroundShade;
    shadeColor.A = FMath::Min(shadeColor.A, 0.06f);
    Shade->SetBrushColor(shadeColor);
    UOverlaySlot* ShadeSlot = RootOverlay->AddChildToOverlay(Shade);
    ShadeSlot->SetHorizontalAlignment(HAlign_Fill);
    ShadeSlot->SetVerticalAlignment(VAlign_Fill);
    UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Menu"));
    m_menu = Menu;
    UOverlaySlot* MenuSlot = RootOverlay->AddChildToOverlay(Menu);
    MenuSlot->SetHorizontalAlignment(HAlign_Left);
    MenuSlot->SetVerticalAlignment(VAlign_Center);
    MenuSlot->SetPadding(m_menuPadding);
    //作品名と操作メニューの前に、三画面共通の短い状況表示を置く。
    UTextBlock* chapter = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SceneChapter"));
    const TCHAR* chapterText = _screen == EGameFlowScreen::Start ? TEXT("SURVIVAL / INFECTED FOREST") :
        (_screen == EGameFlowScreen::Clear ? TEXT("EXTRACTION / DAYBREAK") : TEXT("SIGNAL LOST / IN THE FOREST"));
    chapter->SetText(FText::AsCultureInvariant(chapterText));
    chapter->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 13, TEXT("Bold")));
    chapter->SetColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.75f, 0.72f)));
    Menu->AddChildToVerticalBox(chapter)->SetPadding(FMargin(2, 0, 0, 18));
    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
    Title->SetText(m_titleOverride.IsEmpty() ? GetTitle(_screen) : m_titleOverride);
    Title->SetColorAndOpacity(
        FSlateColor(_screen == EGameFlowScreen::GameOver ? FLinearColor(0.82f, 0.04f, 0.03f, 1.f) : FLinearColor(0.92f, 0.94f, 0.91f, 1.f)));
    Title->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), FMath::Min(m_titleFontSize, 48), TEXT("Bold")));
    Title->SetShadowOffset(FVector2D(3.f, 3.f));
    Title->SetShadowColorAndOpacity(FLinearColor::Black);
    Menu->AddChildToVerticalBox(Title);
    UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Subtitle"));
    Subtitle->SetText(m_subtitleOverride.IsEmpty() ? GetSubtitle(_screen) : m_subtitleOverride);
    Subtitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.76f, 0.78f, 1.f)));
    Subtitle->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), m_subtitleFontSize));
    UVerticalBoxSlot* SubtitleSlot = Menu->AddChildToVerticalBox(Subtitle);
    SubtitleSlot->SetPadding(FMargin(2.f, 4.f, 0.f, 30.f));

    const FText PrimaryLabel =
        _screen == EGameFlowScreen::Start ? FText::AsCultureInvariant(TEXT("ENTER THE FOREST")) : FText::AsCultureInvariant(TEXT("PLAY AGAIN"));
    m_pPrimaryButton = AddMenuButton(Menu, PrimaryLabel, TEXT("PrimaryButton"));
    m_pPrimaryButton->OnClicked.AddDynamic(this, &UGameFlowScreenWidget::HandlePrimaryAction);
    if (_screen != EGameFlowScreen::Start)
    {
        m_pTitleButton = AddMenuButton(Menu, FText::AsCultureInvariant(TEXT("BACK TO TITLE")), TEXT("TitleButton"));
        m_pTitleButton->OnClicked.AddDynamic(this, &UGameFlowScreenWidget::HandleTitleAction);
    }

    m_pQuitButton = AddMenuButton(Menu, FText::AsCultureInvariant(TEXT("QUIT")), TEXT("QuitButton"));
    m_pQuitButton->OnClicked.AddDynamic(this, &UGameFlowScreenWidget::HandleQuitAction);
    UTextBlock* ControllerHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControllerHint"));
    ControllerHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.48f, 0.72f, 0.70f, 1.0f)));
    ControllerHint->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 13));
    UVerticalBoxSlot* HintSlot = Menu->AddChildToVerticalBox(ControllerHint);
    HintSlot->SetPadding(FMargin(2.f, 12.f, 0.f, 0.f));
    ControllerHint->SetText(FText::AsCultureInvariant(TEXT("ENTER / A  SELECT     ARROWS / D-PAD  NAVIGATE")));

    //場面の目標が一目で伝わる短い案内を、操作メニューと離して置く
    UTextBlock* objective = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SceneObjective"));
    const TCHAR* message = _screen == EGameFlowScreen::Start ? TEXT("CLEAR THE FOREST\nFind the rifle. Defeat the infected. Reach the gate.") :
        (_screen == EGameFlowScreen::Clear ? TEXT("THE GATE IS OPEN\nYou made it out of the forest.") :
                                             TEXT("ONE MORE CHANCE\nKeep your distance. Reload before they close in."));
    objective->SetText(FText::AsCultureInvariant(message));
    objective->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 16));
    objective->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.68f, 0.65f)));
    UOverlaySlot* objectiveSlot = RootOverlay->AddChildToOverlay(objective);
    objectiveSlot->SetHorizontalAlignment(HAlign_Right);
    objectiveSlot->SetVerticalAlignment(VAlign_Bottom);
    objectiveSlot->SetPadding(FMargin(40, 0, 50, 55));
    m_fade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TransitionFade"));
    m_fade->SetBrushColor(FLinearColor::Black);
    m_fade->SetRenderOpacity(0.0f);
    m_fade->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* fadeSlot = RootOverlay->AddChildToOverlay(m_fade);
    fadeSlot->SetHorizontalAlignment(HAlign_Fill);
    fadeSlot->SetVerticalAlignment(VAlign_Fill);
    m_buttonWeights.Init(0.0f, 3);
}

//画面の入りと選択の動きを抑えめに揃え、文字を追いかけなくても操作できるようにする
void UGameFlowScreenWidget::NativeTick(const FGeometry& _geometry, float _deltaTime)
{
    Super::NativeTick(_geometry, _deltaTime);
    m_screenTime += _deltaTime;
    if (m_menu)
    {
        const float progress = FMath::Clamp(m_screenTime / 0.65f, 0.0f, 1.0f);
        const float eased = 1.0f - FMath::Pow(1.0f - progress, 3.0f);
        m_menu->SetRenderOpacity(eased);
        m_menu->SetRenderTranslation(FVector2D(-28.0f * (1.0f - eased), 0.0f));
    }
    UButton* buttons[] = {m_pPrimaryButton, m_pTitleButton, m_pQuitButton};
    int32 focused = INDEX_NONE;
    for (int32 index = 0; index < 3; ++index)
    {
        UButton* button = buttons[index];
        if (!button || !m_buttonWeights.IsValidIndex(index)) { continue; }
        const bool selected = button->IsHovered() || button->HasKeyboardFocus() ||
                              (GetOwningPlayer() && button->HasUserFocus(GetOwningPlayer()));
        m_buttonWeights[index] = FMath::FInterpTo(m_buttonWeights[index], selected ? 1.0f : 0.0f, _deltaTime, 12.0f);
        const float weight = m_buttonWeights[index];
        button->SetRenderTranslation(FVector2D(8.0f * weight, 0.0f));
        button->SetBackgroundColor(FMath::Lerp(FLinearColor(0.025f, 0.035f, 0.04f), FLinearColor(0.24f, 0.09f, 0.07f), weight));
        if (selected) { focused = index; }
    }
    if (focused != INDEX_NONE && focused != m_focusedIndex && m_focusedIndex != INDEX_NONE) { PlaySelectSound(); }
    m_focusedIndex = focused;
    if (!m_pendingLevel.IsNone())
    {
        m_exitTime += _deltaTime;
        if (m_fade) { m_fade->SetRenderOpacity(FMath::Clamp(m_exitTime / 0.25f, 0.0f, 1.0f)); }
        if (m_exitTime >= 0.25f)
        {
            const FName level = m_pendingLevel;
            m_pendingLevel = NAME_None;
            UGameplayStatics::OpenLevel(this, level);
        }
    }
}

//キーボードとゲームパッド操作の初期フォーカスを主要ボタンへ移します。
void UGameFlowScreenWidget::FocusDefaultButton(APlayerController* _playerController)
{
    if (!m_pPrimaryButton || !_playerController) { return; }

    m_pPrimaryButton->SetUserFocus(_playerController);
}

//初期ButtonFocusを所有しているか判定します。
bool UGameFlowScreenWidget::HasDefaultButtonFocus(APlayerController* _playerController) const
{
    return m_pPrimaryButton && _playerController && m_pPrimaryButton->HasUserFocus(_playerController);
}

//MenuButtonを現在の所持内容へ追加します。
UButton* UGameFlowScreenWidget::AddMenuButton(UVerticalBox* _parent, const FText& _label, FName _widgetName)
{
    //配置先のVerticalBoxがない場合はWidgetを構築できないため、参照なしを返して終了します。
    if (!_parent) { return nullptr; }
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), _widgetName);
    Button->SetBackgroundColor(FLinearColor(0.025f, 0.035f, 0.04f, 0.92f));
    Button->SetColorAndOpacity(FLinearColor::White);
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(_label);
    Label->SetJustification(ETextJustify::Left);
    Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.9f, 0.9f, 1.f)));
    Label->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 21, TEXT("Bold")));
    Label->SetMargin(FMargin(20.f, 12.f));
    Button->SetContent(Label);
    UVerticalBoxSlot* ButtonSlot = _parent->AddChildToVerticalBox(Button);
    ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
    ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
    return Button;
}

//LevelCheckedへ安全に遷移します。
void UGameFlowScreenWidget::OpenLevelChecked(FName _levelName)
{
    if (_levelName.IsNone()) { return; }

    if (!m_pendingLevel.IsNone()) { return; }
    m_pendingLevel = _levelName;
    m_exitTime = 0.0f;
    if (m_menu) { m_menu->SetIsEnabled(false); }
}

//PrimaryActionの通知を受けてゲーム状態へ反映します。
void UGameFlowScreenWidget::HandlePrimaryAction()
{
    PlaySelectSound();
    OpenLevelChecked(GameplayLevelName);
}

//TitleActionの通知を受けてゲーム状態へ反映します。
void UGameFlowScreenWidget::HandleTitleAction()
{
    PlaySelectSound();
    OpenLevelChecked(TitleLevelName);
}

//QuitActionの通知を受けてゲーム状態へ反映します。
void UGameFlowScreenWidget::HandleQuitAction()
{
    PlaySelectSound();
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
    }
}

//Selectサウンドを再生します。
void UGameFlowScreenWidget::PlaySelectSound() const
{
    constexpr TCHAR SelectSoundPath[] = TEXT("/Game/Audio/CC0/S_UI_Select.S_UI_Select");
    if (USoundBase* SelectSound = LoadObject<USoundBase>(nullptr, SelectSoundPath))
    {
        UGameplayStatics::PlaySound2D(this, SelectSound);
    }
}
