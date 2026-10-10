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

    //持ち替え・リロード・照準の間だけ上半身スロットを合成し、脚は移動ポーズを使う。
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float m_upperBodyWeight = 0.0f;
};
