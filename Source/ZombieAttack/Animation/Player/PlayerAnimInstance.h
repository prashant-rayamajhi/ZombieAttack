#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnimInstance.generated.h"

//既存の武器別AnimBPへ、前進と後退を区別した移動速度を渡す。
UCLASS()
class ZOMBIEATTACK_API UPlayerAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    //移動アニメーションを選ぶ前に、見た目の正面と実際の速度を照合する。
    virtual void NativeUpdateAnimation(float _deltaSeconds) override;

    //武器別BlendSpaceの横軸。後退は負、前進と横移動は正の速さで再生する。
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    float m_locomotionSpeed = 0.0f;
};
