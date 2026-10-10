#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFlowGameMode.generated.h"

//UAudioComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAudioComponent;
//USoundBaseは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class USoundBase;

//ゲームプレイを行わない画面専用の軽量GameModeです。
//メニュー入力用PlayerControllerだけを残し、ゲーム用PawnとHUDは生成しません。
//開始画面と結果画面の背後へ不要なゲーム要素が生成されることを防ぎます。
UCLASS()
class ZOMBIEATTACK_API AGameFlowGameMode : public AGameModeBase
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    AGameFlowGameMode();

  protected:
    //StartPlayは、名前が示す動作を開始するための初期状態を整えます。
    virtual void StartPlay() override;
    //自動破棄しないBGMを停止し、GameMode終了後に音源を残さない。
    virtual void EndPlay(const EEndPlayReason::Type _reason) override;
    //HandleStartingNewPlayer_Implementationは、参加したPlayerControllerの開始処理を行い、ゲーム用PawnまたはUIを準備します。
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* _newPlayer) override;
    virtual void RestartPlayer(AController* _newPlayer) override;

  private:
    //画面ごとのBGMを選択し、別レベルへ持ち越さずに再生します。
    void StartScreenMusic();

    //gameStartMusicをゲーム処理から参照できるように管理します。
    UPROPERTY(EditDefaultsOnly, Category = "Audio|Music")
    //gameStartMusicをゲーム処理から参照できるように管理します。
    TSoftObjectPtr<USoundBase> m_gameStartMusic;

    //gameClearMusicをゲーム処理から参照できるように管理します。
    UPROPERTY(EditDefaultsOnly, Category = "Audio|Music")
    //gameClearMusicをゲーム処理から参照できるように管理します。
    TSoftObjectPtr<USoundBase> m_gameClearMusic;

    //gameOverMusicをゲーム処理から参照できるように管理します。
    UPROPERTY(EditDefaultsOnly, Category = "Audio|Music")
    //gameOverMusicをゲーム処理から参照できるように管理します。
    TSoftObjectPtr<USoundBase> m_gameOverMusic;

    UPROPERTY(EditDefaultsOnly, Category = "Audio|Music", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    //screenMusicVolumeをゲーム処理から参照できるように管理します。
    float m_screenMusicVolume;

    //screenMusicComponentをゲーム処理から参照できるように管理します。
    UPROPERTY(Transient)
    //screenMusicComponentをゲーム処理から参照できるように管理します。
    TObjectPtr<UAudioComponent> m_screenMusicComponent;
};
