#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFlowScreenWidget.generated.h"

//UButtonは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UButton;
//UVerticalBoxは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UVerticalBox;
class UBorder;

//GameStart / GameClear / GameOver 共通のフルスクリーンUIです。
//背景は描画Widgetで構成し、生成画像やLevel Blueprintの設定に依存させない。
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
    bool HasDefaultButtonFocus(APlayerController* _playerController) const;

  protected:
    //NativeOnInitializedは、Widget初期化時にイベント接続と固定UI要素の準備を行います。
    virtual void NativeOnInitialized() override;
    //入力先の準備前に作られた場合も、Slate構築時には必ず画面を用意する
    virtual TSharedRef<SWidget> RebuildWidget() override;
    //入場、フォーカス、画面遷移の短いアニメーションを更新する
    virtual void NativeTick(const FGeometry& _geometry, float _deltaTime) override;

  private:
    //レベル名から開始、クリア、敗北の画面構成を選ぶ
    void InitializeScreen();
    //メニューを左から静かに入場させるための参照
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> m_menu;
    //レベル遷移時のフェードを最前面へ描く
    UPROPERTY(Transient)
    TObjectPtr<UBorder> m_fade;
    //画面を表示してからの経過秒数
    float m_screenTime = 0.0f;
    //各ボタンの選択強調を補間する
    TArray<float> m_buttonWeights;
    //選択音を連続再生しないための直前の選択位置
    int32 m_focusedIndex = INDEX_NONE;
    //フェード終了後に開くレベル
    FName m_pendingLevel;
    //レベル遷移前の暗転時間
    float m_exitTime = 0.0f;
    //Screenを作成します。
    void BuildScreen(EGameFlowScreen _screen);
    //AddMenuButtonは、名前が示す対象を既存の状態または一覧へ追加します。
    UButton* AddMenuButton(UVerticalBox* _parent, const FText& _label, FName _widgetName);
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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Style", meta = (AllowPrivateAccess = "true", ClampMin = "8", ClampMax = "160"))
    //titleFontSizeをゲーム処理から参照できるように管理します。
    int32 m_titleFontSize = 58;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Style", meta = (AllowPrivateAccess = "true", ClampMin = "8", ClampMax = "80"))
    //subtitleFontSizeをゲーム処理から参照できるように管理します。
    int32 m_subtitleFontSize = 19;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Style", meta = (AllowPrivateAccess = "true"))
    FMargin m_menuPadding = FMargin(96.f, 24.f, 0.f, 24.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Style", meta = (AllowPrivateAccess = "true"))
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
