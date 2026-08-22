#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameFlowPlayerController.generated.h"

//タイトル・クリア・ゲームオーバー画面専用Controller。
//初回起動時も確実にUIを生成するため、MapロードDelegateだけに依存せず
//PlayerControllerのBeginPlayから画面を保証します。
UCLASS()
class ZOMBIEATTACK_API AGameFlowPlayerController : public APlayerController
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  protected:
    //ゲーム開始時の初期設定を行います。
    virtual void BeginPlay() override;

  private:
    //EnsureGameFlowScreenは、名前が示す対象が未生成の場合だけ作成し、利用可能な状態を保証します。
    void EnsureGameFlowScreen();
};
