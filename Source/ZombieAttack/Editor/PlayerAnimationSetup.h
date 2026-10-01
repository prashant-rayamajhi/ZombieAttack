#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "PlayerAnimationSetup.generated.h"

//既存AnimBPの移動入力だけを差し替える、エディタ用の一括設定コマンド。
UCLASS()
class UPlayerAnimationSetupCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    //武器別BlendSpaceへ符号付き速度を接続し、待機やジャンプの条件は残す。
    virtual int32 Main(const FString& _params) override;
};
