#include "WeaponAimTrace.h"
#include "Engine/World.h"

//敵のVisibility設定に左右されず、壁を通り越して敵だけを選ばない射線判定。
bool WeaponAimTrace::FindFirstHit(UWorld* _world, const FVector& _start, const FVector& _end,
    AActor* _owner, AActor* _weapon, FHitResult& _hit)
{
    _hit = FHitResult();
    if (!_world) { return false; }
    //自分の体と手に持った武器を命中候補から外す。
    FCollisionQueryParams query(SCENE_QUERY_STAT(WeaponLineOfFire), true);
    query.AddIgnoredActor(_owner);
    query.AddIgnoredActor(_weapon);
    //遮蔽物を先に調べ、敵が背後にいる場合の比較距離を得る。
    const bool blocked = _world->LineTraceSingleByChannel(_hit, _start, _end, ECC_Visibility, query);
    FCollisionObjectQueryParams pawnObjects(ECC_Pawn);
    FHitResult pawnHit;
    const bool hitPawn = _world->LineTraceSingleByObjectType(pawnHit, _start, _end, pawnObjects, query);
    if (hitPawn && (!blocked || pawnHit.Time < _hit.Time)) { _hit = pawnHit; }
    return blocked || hitPawn;
}
