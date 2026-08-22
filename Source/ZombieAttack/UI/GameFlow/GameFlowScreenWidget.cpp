#include "GameFlowScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
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
    //現在の状態に合う処理へ分けます。
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
    //現在の状態に合う処理へ分けます。
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
    : Super(_objectInitializer), m_gameStartBackground(FSoftObjectPath(TEXT("/Game/UI/Generated/T_GameStart_Background.T_GameStart_Background"))),
      m_gameClearBackground(FSoftObjectPath(TEXT("/Game/UI/Generated/T_GameClear_Background.T_GameClear_Background"))),
      m_gameOverBackground(FSoftObjectPath(TEXT("/Game/UI/Generated/T_GameOver_Background.T_GameOver_Background")))
{
}

//Widget生成時に子Widgetとゲーム側の通知を接続します。
void UGameFlowScreenWidget::NativeOnInitialized()
{
    //Widget生成時に子Widgetとゲーム側の通知を接続します。
    Super::NativeOnInitialized();

    //LevelNameは、UGameplayStatics::GetCurrentLevelName(this, true)から構築した結果を後続の処理へ渡すために使います。
    const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
    //「LevelName.Equals(TEXT("GameClear"), ESearchCase::IgnoreCase)」が成立するとき、m_screenを更新します。
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

    //「WidgetTree && WidgetTree->RootWidget」が成立するとき、BindDesignerWidgetsを呼び出します。
    if (WidgetTree && WidgetTree->RootWidget)
    {
        BindDesignerWidgets();
    }
    else
    {
        BuildScreen(m_screen);
    }
}

//Screenを作成します。
void UGameFlowScreenWidget::BuildScreen(EGameFlowScreen _screen)
{
    //「!WidgetTree」が成立するとき、TEXTを呼び出します。
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("GameFlowWidgetTree"));
    }

    //RootOverlayは、WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Ro…から取得した参照を後続の呼び出しで使います。
    UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
    WidgetTree->RootWidget = RootOverlay;

    //Backgroundは、WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Backgr…から取得した参照を後続の呼び出しで使います。
    UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Background"));
    //「UTexture2D* Texture = LoadBackground(_screen)」が成立するとき、SetBrushFromTextureを呼び出します。
    if (UTexture2D* Texture = LoadBackground(_screen))
    {
        Background->SetBrushFromTexture(Texture, true);
    }
    Background->SetBrushTintColor(FSlateColor(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f)));
    //BackgroundSlotは、RootOverlay->AddChildToOverlay(Background)から取得した参照を後続の呼び出しで使います。
    UOverlaySlot* BackgroundSlot = RootOverlay->AddChildToOverlay(Background);
    BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
    BackgroundSlot->SetVerticalAlignment(VAlign_Fill);

    //背景を少し暗くして、どの解像度でも文字の可読性を保ちます。
    UBorder* Shade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundShade"));
    Shade->SetBrushColor(m_backgroundShade);
    //ShadeSlotは、RootOverlay->AddChildToOverlay(Shade)から取得した参照を後続の呼び出しで使います。
    UOverlaySlot* ShadeSlot = RootOverlay->AddChildToOverlay(Shade);
    ShadeSlot->SetHorizontalAlignment(HAlign_Fill);
    ShadeSlot->SetVerticalAlignment(VAlign_Fill);

    //Menuは、WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), …から取得した参照を後続の呼び出しで使います。
    UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Menu"));
    //MenuSlotは、RootOverlay->AddChildToOverlay(Menu)から取得した参照を後続の呼び出しで使います。
    UOverlaySlot* MenuSlot = RootOverlay->AddChildToOverlay(Menu);
    MenuSlot->SetHorizontalAlignment(HAlign_Left);
    MenuSlot->SetVerticalAlignment(VAlign_Center);
    MenuSlot->SetPadding(m_menuPadding);

    //Titleは、WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT…から取得した参照を後続の呼び出しで使います。
    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
    Title->SetText(m_titleOverride.IsEmpty() ? GetTitle(_screen) : m_titleOverride);
    Title->SetColorAndOpacity(
        FSlateColor(_screen == EGameFlowScreen::GameOver ? FLinearColor(0.82f, 0.04f, 0.03f, 1.f) : FLinearColor(0.92f, 0.94f, 0.91f, 1.f)));
    Title->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), m_titleFontSize, TEXT("Bold")));
    Title->SetShadowOffset(FVector2D(3.f, 3.f));
    Title->SetShadowColorAndOpacity(FLinearColor::Black);
    Menu->AddChildToVerticalBox(Title);

    //Subtitleは、WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT…から取得した参照を後続の呼び出しで使います。
    UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Subtitle"));
    Subtitle->SetText(m_subtitleOverride.IsEmpty() ? GetSubtitle(_screen) : m_subtitleOverride);
    Subtitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.76f, 0.78f, 1.f)));
    Subtitle->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), m_subtitleFontSize));
    //SubtitleSlotは、Menu->AddChildToVerticalBox(Subtitle)から取得した参照を後続の呼び出しで使います。
    UVerticalBoxSlot* SubtitleSlot = Menu->AddChildToVerticalBox(Subtitle);
    SubtitleSlot->SetPadding(FMargin(2.f, 4.f, 0.f, 30.f));

    const FText PrimaryLabel =
        _screen == EGameFlowScreen::Start ? FText::AsCultureInvariant(TEXT("GAME START")) : FText::AsCultureInvariant(TEXT("RETRY"));
    m_pPrimaryButton = AddMenuButton(Menu, PrimaryLabel, TEXT("PrimaryButton"));
    m_pPrimaryButton->OnClicked.AddDynamic(this, &UGameFlowScreenWidget::HandlePrimaryAction);

    //「_screen != EGameFlowScreen::Start」が成立するとき、AddMenuButtonを呼び出します。
    if (_screen != EGameFlowScreen::Start)
    {
        m_pTitleButton = AddMenuButton(Menu, FText::AsCultureInvariant(TEXT("BACK TO TITLE")), TEXT("TitleButton"));
        m_pTitleButton->OnClicked.AddDynamic(this, &UGameFlowScreenWidget::HandleTitleAction);
    }

    m_pQuitButton = AddMenuButton(Menu, FText::AsCultureInvariant(TEXT("QUIT")), TEXT("QuitButton"));
    m_pQuitButton->OnClicked.AddDynamic(this, &UGameFlowScreenWidget::HandleQuitAction);

    //ControllerHintは、WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT…から取得した参照を後続の呼び出しで使います。
    UTextBlock* ControllerHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControllerHint"));
    ControllerHint->SetText(FText::AsCultureInvariant(TEXT("A  SELECT・LEFT STICK ・ D-PAD  NAVIGATE")));
    ControllerHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.48f, 0.72f, 0.70f, 1.0f)));
    ControllerHint->SetText(FText::AsCultureInvariant(TEXT("A  SELECT  /  LEFT STICK OR D-PAD  NAVIGATE")));
    ControllerHint->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 13));
    //HintSlotは、Menu->AddChildToVerticalBox(ControllerHint)から取得した参照を後続の呼び出しで使います。
    UVerticalBoxSlot* HintSlot = Menu->AddChildToVerticalBox(ControllerHint);
    HintSlot->SetPadding(FMargin(2.f, 12.f, 0.f, 0.f));
}

//binddesignerwidgetsを登録します。
void UGameFlowScreenWidget::BindDesignerWidgets()
{
    m_pPrimaryButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("PrimaryButton")));
    m_pTitleButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("TitleButton")));
    m_pQuitButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("QuitButton")));

    //「m_pPrimaryButton」が成立するとき、AddUniqueDynamicを呼び出します。
    if (m_pPrimaryButton)
    {
        m_pPrimaryButton->OnClicked.AddUniqueDynamic(this, &UGameFlowScreenWidget::HandlePrimaryAction);
    }
    //「m_pTitleButton」が成立するとき、AddUniqueDynamicを呼び出します。
    if (m_pTitleButton)
    {
        m_pTitleButton->OnClicked.AddUniqueDynamic(this, &UGameFlowScreenWidget::HandleTitleAction);
    }
    //「m_pQuitButton」が成立するとき、AddUniqueDynamicを呼び出します。
    if (m_pQuitButton)
    {
        m_pQuitButton->OnClicked.AddUniqueDynamic(this, &UGameFlowScreenWidget::HandleQuitAction);
    }
}

//キーボードとゲームパッド操作の初期フォーカスを主要ボタンへ移します。
void UGameFlowScreenWidget::FocusDefaultButton(APlayerController* _playerController)
{
    //「!m_pPrimaryButton || !_playerController」が成立するとき、SetUserFocusを呼び出します。
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

    //Buttonは、WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), _widgetNam…から取得した参照を後続の呼び出しで使います。
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), _widgetName);
    Button->SetBackgroundColor(FLinearColor(0.025f, 0.035f, 0.04f, 0.92f));
    Button->SetColorAndOpacity(FLinearColor::White);

    //Labelは、WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass())から取得した参照を後続の呼び出しで使います。
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(_label);
    Label->SetJustification(ETextJustify::Left);
    Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.9f, 0.9f, 1.f)));
    Label->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 21, TEXT("Bold")));
    Label->SetMargin(FMargin(20.f, 12.f));
    Button->SetContent(Label);

    //ButtonSlotは、_parent->AddChildToVerticalBox(Button)から取得した参照を後続の呼び出しで使います。
    UVerticalBoxSlot* ButtonSlot = _parent->AddChildToVerticalBox(Button);
    ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
    ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
    //Buttonは、UI部品を構築または更新する呼び出しで参照するために使います。
    return Button;
}

//BackgroundをAssetから読み込みます。
UTexture2D* UGameFlowScreenWidget::LoadBackground(EGameFlowScreen _screen) const
{
    //「m_backgroundOverride」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (m_backgroundOverride) { return m_backgroundOverride; }

    //現在の状態に合う処理へ分けます。
    switch (_screen)
    {
    case EGameFlowScreen::Clear: return m_gameClearBackground.LoadSynchronous();
    case EGameFlowScreen::GameOver: return m_gameOverBackground.LoadSynchronous();
    default: return m_gameStartBackground.LoadSynchronous();
    }
}

//LevelCheckedへ安全に遷移します。
void UGameFlowScreenWidget::OpenLevelChecked(FName _levelName)
{
    if (_levelName.IsNone())
    {
        return;
    }

    SetIsEnabled(false);
    //Levelへ安全に遷移します。
    UGameplayStatics::OpenLevel(this, _levelName);
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
    //「APlayerController* PlayerController = GetOwningPlayer()」が成立するとき、QuitGameを呼び出します。
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
    }
}

//Selectサウンドを再生します。
void UGameFlowScreenWidget::PlaySelectSound() const
{
    constexpr TCHAR SelectSoundPath[] = TEXT("/Game/Audio/CC0/S_UI_Select.S_UI_Select");
    //「USoundBase* SelectSound = LoadObject<USoundBase>(nullptr, SelectSoundPath)」が成立するとき、PlaySound2Dを呼び出します。
    if (USoundBase* SelectSound = LoadObject<USoundBase>(nullptr, SelectSoundPath))
    {
        UGameplayStatics::PlaySound2D(this, SelectSound);
    }
}
