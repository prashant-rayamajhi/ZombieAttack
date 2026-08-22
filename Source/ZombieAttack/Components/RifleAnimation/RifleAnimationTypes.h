#pragma once

#include "CoreMinimal.h"
#include "RifleAnimationTypes.generated.h"

//ARの見た目上の状態です。武器処理とAnimBPの双方から同じ状態を参照します。
UENUM(BlueprintType)
enum class ERifleAimState : uint8
{
    //銃を下ろしている通常状態
    Inactive UMETA(DisplayName = "Inactive"),
    //銃を構えている途中の状態
    Raising UMETA(DisplayName = "Raising"),
    //照準が整い発砲できる状態
    Ready UMETA(DisplayName = "Ready"),
    //リロード中を表す項目
    Reloading UMETA(DisplayName = "Reloading")
};
