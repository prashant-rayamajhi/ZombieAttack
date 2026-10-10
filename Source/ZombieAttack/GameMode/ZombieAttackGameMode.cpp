#include "ZombieAttackGameMode.h"

#include "../Player/PlayerChara.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

//AZombieAttackGameModeが使用するComponentと初期パラメータを設定します。
AZombieAttackGameMode::AZombieAttackGameMode()
{
    //PlayerClassは、名前が示す動作を開始するための初期状態を整えます。
    static ConstructorHelpers::FClassFinder<APlayerChara> PlayerClass(TEXT("/Game/Blueprints/Player/Actor/BP_PlayerChara"));
    if (PlayerClass.Succeeded())
    {
        DefaultPawnClass = PlayerClass.Class;
    }
}

//本編開始時にゲームプレイ用BGMを生成し、音量設定を反映します。
void AZombieAttackGameMode::StartPlay()
{
    //GameMode共通の開始処理を先に完了させ、プレイヤーとレベルActorを利用可能にします。
    Super::StartPlay();
    USoundBase* GameplayMusic = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Music/S_BGM_TheHunt.S_BGM_TheHunt"));
    //BGMアセットを読み込めた場合だけ、ゲームプレイ中の2D音源を生成します。
    if (GameplayMusic)
    {
        m_gameplayMusicComponent = UGameplayStatics::SpawnSound2D(this, GameplayMusic, 0.24f, 1.0f, 0.0f, nullptr, false, false);
    }
}

//同じワールド内でGameModeを破棄しても、再生中のBGMと登録済み音源を回収する。
void AZombieAttackGameMode::EndPlay(const EEndPlayReason::Type _reason)
{
    if (IsValid(m_gameplayMusicComponent))
    {
        m_gameplayMusicComponent->Stop();
        m_gameplayMusicComponent->DestroyComponent();
    }
    m_gameplayMusicComponent = nullptr;
    Super::EndPlay(_reason);
}

//参加したプレイヤーを本編用Pawnへ割り当てます。
void AZombieAttackGameMode::HandleStartingNewPlayer_Implementation(APlayerController* _newPlayer)
{
    //参加したプレイヤーを本編用Pawnへ割り当てます。
    Super::HandleStartingNewPlayer_Implementation(_newPlayer);
    if (!_newPlayer) { return; }

    //タイトル画面のUIOnly状態や入力無視状態を本編へ持ち越さないようにします。
    _newPlayer->ResetIgnoreMoveInput();
    _newPlayer->ResetIgnoreLookInput();
    _newPlayer->SetInputMode(FInputModeGameOnly());
    _newPlayer->SetShowMouseCursor(false);
}
