#pragma once
#include "CoreMinimal.h"
class UWorld;
class AActor;

//照準と銃口の二段階で射線を調べ、肩越し視点で壁越しに命中するのを防ぐ。
namespace WeaponAimTrace
{
    //壁と敵を別々に調べ、最も手前の衝突だけを命中結果として返す。
    bool FindFirstHit(UWorld* _world, const FVector& _start, const FVector& _end,
        AActor* _owner, AActor* _weapon, FHitResult& _hit);
}
