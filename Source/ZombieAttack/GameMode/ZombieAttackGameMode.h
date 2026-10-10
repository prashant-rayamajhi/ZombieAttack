#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZombieAttackGameMode.generated.h"

//UAudioComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAudioComponent;

//GameLevel1専用の本編GameModeです。
UCLASS()
class ZOMBIEATTACK_API AZombieAttackGameMode : public AGameModeBase
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    AZombieAttackGameMode();

  protected:
    //StartPlayは、名前が示す動作を開始するための初期状態を整えます。
    virtual void StartPlay() override;
    //自動破棄しないBGMを停止し、GameMode終了後に音源を残さない。
    virtual void EndPlay(const EEndPlayReason::Type _reason) override;
    //HandleStartingNewPlayer_Implementationは、参加したPlayerControllerの開始処理を行い、ゲーム用PawnまたはUIを準備します。
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* _newPlayer) override;

  private:
    UPROPERTY(Transient)
    //m_gameplayMusicComponentは、本編BGMを停止または音量調整できるよう再生中のAudioComponentを保持します。
    TObjectPtr<UAudioComponent> m_gameplayMusicComponent;
};
