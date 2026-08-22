#include "GameFlowGameMode.h"
#include "GameFlowPlayerController.h"
#include "Components/AudioComponent.h"
#include "GameFramework/HUD.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

//AGameFlowGameModeが使用するComponentと初期パラメータを設定します。
AGameFlowGameMode::AGameFlowGameMode()
    : m_gameStartMusic(FSoftObjectPath(TEXT("/Game/Audio/CC0/Music/BGM_GameStart_AmbientHorror.BGM_GameStart_AmbientHorror"))),
      m_gameClearMusic(FSoftObjectPath(TEXT("/Game/Audio/CC0/Music/BGM_GameClear_CreedOfCourse.BGM_GameClear_CreedOfCourse"))),
      m_gameOverMusic(FSoftObjectPath(TEXT("/Game/Audio/CC0/Music/BGM_GameOver_Alone.BGM_GameOver_Alone"))), m_screenMusicVolume(0.32f),
      m_screenMusicComponent(nullptr)
{
    //軽量な非表示Spectatorを生成し、エンジンの開始処理に必要なPawnを用意します。
    //このマップでは全画面UIが入力と表示を管理します。
    DefaultPawnClass = ASpectatorPawn::StaticClass();
    HUDClass = AHUD::StaticClass();
    PlayerControllerClass = AGameFlowPlayerController::StaticClass();
}

//画面表示開始時に共通初期化と画面別BGMの再生を行います。
void AGameFlowGameMode::StartPlay()
{
    //GameMode共通の開始処理を完了させてから、画面に対応するBGMを開始します。
    Super::StartPlay();
    StartScreenMusic();
}

//参加したプレイヤーを本編用Pawnへ割り当てます。
void AGameFlowGameMode::HandleStartingNewPlayer_Implementation(APlayerController* _newPlayer)
{
    //メニューと結果マップではPawnとPlayerStartを生成せず、Controllerだけを維持します。
    //画面遷移ごとに不要なPawn生成失敗警告が出ることを防ぎます。
}

//本編以外の画面ではPawnを生成せず、UI操作だけを維持します。
void AGameFlowGameMode::RestartPlayer(AController* _newPlayer)
{
    //Start、Clear、OverはUI専用マップのため、ボタン入力用Controllerだけを生成します。
}

//画面Musicを開始します。
void AGameFlowGameMode::StartScreenMusic()
{
    //「!GetWorld()」が成立するとき、GetWorldを呼び出します。
    if (!GetWorld()) { return; }

    //ワールドを返します。
    const FString mapName = GetWorld()->GetMapName();
    //selectedMusicは、&m_gameStartMusicから取得した参照を後続の呼び出しで使います。
    TSoftObjectPtr<USoundBase>* selectedMusic = &m_gameStartMusic;
    //screenNameは、TEXT("GameStart")から取得した参照を後続の呼び出しで使います。
    const TCHAR* screenName = TEXT("GameStart");
    //「mapName.Contains(TEXT("GameClear"))」が成立するとき、selectedMusicを更新します。
    if (mapName.Contains(TEXT("GameClear")))
    {
        selectedMusic = &m_gameClearMusic;
        screenName = TEXT("GameClear");
    }
    else if (mapName.Contains(TEXT("GameOver")))
    {
        selectedMusic = &m_gameOverMusic;
        screenName = TEXT("GameOver");
    }

    //musicは、selectedMusic->LoadSynchronous()から取得した参照を後続の呼び出しで使います。
    USoundBase* music = selectedMusic->LoadSynchronous();
    if (!music)
    {
        return;
    }

    //レベル遷移後に前画面の曲が重ならないよう、現在のWorldだけで再生します。
    m_screenMusicComponent = UGameplayStatics::SpawnSound2D(this, music, m_screenMusicVolume, 1.0f, 0.0f, nullptr, false, false);
}
