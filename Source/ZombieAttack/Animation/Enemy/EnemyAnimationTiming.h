#pragma once

#include "CoreMinimal.h"

class UAnimMontage;

//通知付きの攻撃に代替タイマーを重ねないための共通判定
namespace EnemyAnimationTiming
{
    bool HasHitNotify(const UAnimMontage* _montage);
}
