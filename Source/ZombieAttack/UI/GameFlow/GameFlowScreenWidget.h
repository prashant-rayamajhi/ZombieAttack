#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFlowScreenWidget.generated.h"

//UButtonは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UButton;
//UTexture2Dは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UTexture2D;
//UVerticalBoxは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UVerticalBox;

//GameStart / GameClear / GameOver 共通のフルスクリーンUIです。
//背景画像以外はC++で構築し、Level Blueprintの設定漏れに左右されないようにします。
UCLASS(Blueprintable)
class ZOMBIEATTACK_API UGameFlowScreenWidget : public UUserWidget
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    UGameFlowScreenWidget(const FObjectInitializer& _objectInitializer);

    //ゲームFlowScreenの動作をまとめたクラス
    enum class EGameFlowScreen : uint8
    {
        //ゲーム開始画面を表示する
        Start,
        //ゲームクリア画面を表示する
        Clear,
        //ゲームオーバー画面を表示する
        GameOver
    };

    //コントローラーとキーボードのフォーカスを主要ボタンへ移します。
    void FocusDefaultButton(APlayerController* _playerController);

    //HasDefaultButtonFocusは、名前が示す条件の成立可否を呼び出し元へ返します。
    bool HasDefaultButtonFocus(APlayerController* _playerController) const;

  protected:
    //NativeOnInitializedは、Widget初期化時にイベント接続と固定UI要素の準備を行います。
    virtual void NativeOnInitialized() override;

  private:
    //Screenを作成します。
    void BuildScreen(EGameFlowScreen _screen);
    //binddesignerwidgetsを登録します。
    void BindDesignerWidgets();
    //AddMenuButtonは、名前が示す対象を既存の状態または一覧へ追加します。
    UButton* AddMenuButton(UVerticalBox* _parent, const FText& _label, FName _widgetName);
    //LoadBackgroundは、名前が示すアセットまたはレベルを読み込み、利用可能な状態にします。
    UTexture2D* LoadBackground(EGameFlowScreen _screen) const;
    //OpenLevelCheckedは、名前が示すレベルまたは画面へ遷移します。
    void OpenLevelChecked(FName _levelName);
    //PlaySelectSoundは、名前が示す動作を開始するための初期状態を整えます。
    void PlaySelectSound() const;

    //PrimaryActionの通知を受けてゲーム状態へ反映します。
    UFUNCTION()
    void HandlePrimaryAction();

    //TitleActionの通知を受けてゲーム状態へ反映します。
    UFUNCTION()
    void HandleTitleAction();

    //QuitActionの通知を受けてゲーム状態へ反映します。
    UFUNCTION()
    void HandleQuitAction();

  private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Content", meta = (AllowPrivateAccess = "true"))
    //titleOverrideをゲーム処理から参照できるように管理します。
    FText m_titleOverride;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Content", meta = (AllowPrivateAccess = "true"))
    //subtitleOverrideをゲーム処理から参照できるように管理します。
    FText m_subtitleOverride;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Content", meta = (AllowPrivateAccess = "true"))
    //ackgroundOverrideかを示します。
    TObjectPtr<UTexture2D> m_backgroundOverride;

    //Soft参照にすることで、背景画像をCook対象へ確実に含めつつBPから差し替えられます。
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Content", meta = (AllowPrivateAccess = "true"))
    //gameStartBackgroundをゲーム処理から参照できるように管理します。
    TSoftObjectPtr<UTexture2D> m_gameStartBackground;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Content", meta = (AllowPrivateAccess = "true"))
    //gameClearBackgroundをゲーム処理から参照できるように管理します。
    TSoftObjectPtr<UTexture2D> m_gameClearBackground;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Content", meta = (AllowPrivateAccess = "true"))
    //gameOverBackgroundをゲーム処理から参照できるように管理します。
    TSoftObjectPtr<UTexture2D> m_gameOverBackground;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Style", meta = (AllowPrivateAccess = "true", ClampMin = "8", ClampMax = "160"))
    //titleFontSizeをゲーム処理から参照できるように管理します。
    int32 m_titleFontSize = 58;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Style", meta = (AllowPrivateAccess = "true", ClampMin = "8", ClampMax = "80"))
    //subtitleFontSizeをゲーム処理から参照できるように管理します。
    int32 m_subtitleFontSize = 19;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Style", meta = (AllowPrivateAccess = "true"))
    //m_menuPaddingは、FMargin(96.f, 24.f, 0.f, 24.f)から構築した結果を後続の処理へ渡すために使います。
    FMargin m_menuPadding = FMargin(96.f, 24.f, 0.f, 24.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Style", meta = (AllowPrivateAccess = "true"))
    //m_backgroundShadeは、FLinearColor(0.01f, 0.015f, 0.02f, 0.38f)から構築した結果を後続の処理へ渡すために使います。
    FLinearColor m_backgroundShade = FLinearColor(0.01f, 0.015f, 0.02f, 0.38f);

    //screenをゲーム処理から参照できるように管理します。
    EGameFlowScreen m_screen = EGameFlowScreen::Start;

    //PrimaryButtonの操作に使用する参照です。
    UPROPERTY(Transient)
    //PrimaryButtonの操作に使用する参照です。
    TObjectPtr<UButton> m_pPrimaryButton;

    //TitleButtonの操作に使用する参照です。
    UPROPERTY(Transient)
    //TitleButtonの操作に使用する参照です。
    TObjectPtr<UButton> m_pTitleButton;

    //QuitButtonの操作に使用する参照です。
    UPROPERTY(Transient)
    //QuitButtonの操作に使用する参照です。
    TObjectPtr<UButton> m_pQuitButton;
};
