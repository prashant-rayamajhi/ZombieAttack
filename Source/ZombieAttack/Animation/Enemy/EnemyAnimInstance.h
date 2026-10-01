#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemyAnimInstance.generated.h"

//敵の実速度から歩行姿勢を作り、攻撃と咆哮を全身スロットで重ねる
UCLASS(Transient)
class ZOMBIEATTACK_API UEnemyAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

  protected:
    //移動用の評価ノードを個体ごとに用意する
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    //メッシュ破棄時に評価ノードも解放する
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* _proxy) override;
};
