#include "EnemyAnimationTiming.h"
#include "Animation/AnimMontage.h"
#include "ZombieAttack/Animation/AnimNotify_EnemyAttackHit.h"
#include "ZombieAttack/Animation/AnimNotify_BossAttackHit.h"
#include "ZombieAttack/Animation/AnimNotifyState_EnemyAttackCollision.h"

//モンタージュと内包クリップの両方から、実際にダメージを発生させる通知を探す
bool EnemyAnimationTiming::HasHitNotify(const UAnimMontage* _montage)
{
    if (!_montage) { return false; }
    const auto hasHit = [](const UAnimSequenceBase* _animation)
    {
        if (!_animation) { return false; }
        for (const FAnimNotifyEvent& notify : _animation->Notifies)
        {
            if (Cast<UAnimNotify_EnemyAttackHit>(notify.Notify) || Cast<UAnimNotify_BossAttackHit>(notify.Notify) ||
                Cast<UAnimNotifyState_EnemyAttackCollision>(notify.NotifyStateClass)) { return true; }
        }
        return false;
    };
    if (hasHit(_montage)) { return true; }
    for (const FSlotAnimationTrack& track : _montage->SlotAnimTracks)
    {
        for (const FAnimSegment& segment : track.AnimTrack.AnimSegments)
        {
            if (hasHit(segment.GetAnimReference())) { return true; }
        }
    }
    return false;
}
