#pragma once

#include "CoreMinimal.h"

class UAnimSequence;

//足が地面に近い区間の移動量から、一倍速で滑らずに進める速度を測る。
namespace EnemyStride
{
    float MeasureSpeed(const UAnimSequence* _clip, float _fallback);
}
