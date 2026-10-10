#include "EnemyStride.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"

//腰の前進を除いた足の引き戻し量を測り、移動を含むクリップとその場走りを同じ基準にする。
float EnemyStride::MeasureSpeed(const UAnimSequence* _clip, float _fallback)
{
    if (!_clip || !_clip->GetSkeleton() || _clip->GetPlayLength() <= 0.0f) { return _fallback; }
    //左右どちらかの骨がない別形式の素材では、指定した基準速度を維持する。
    const FReferenceSkeleton& skeleton = _clip->GetSkeleton()->GetReferenceSkeleton();
    TArray<float> speeds;
    const int32 samples = 90;
    const float step = _clip->GetPlayLength() / samples;
    for (FName name : {FName(TEXT("LeftFoot")), FName(TEXT("RightFoot"))})
    {
        const int32 foot = skeleton.FindBoneIndex(name);
        if (foot == INDEX_NONE) { continue; }
        //接地時の足の高さを探すため、一周期を一定間隔で採取する。
        TArray<FVector> positions;
        float low = BIG_NUMBER;
        float high = -BIG_NUMBER;
        for (int32 sample = 0; sample <= samples; ++sample)
        {
            FTransform pose = FTransform::Identity;
            for (int32 bone = foot; bone != INDEX_NONE; bone = skeleton.GetParentIndex(bone))
            {
                FTransform local;
                const FAnimExtractContext context(static_cast<double>(sample * step));
                _clip->GetBoneTransform(local, FSkeletonPoseBoneIndex(bone), context, false);
                //最上位の腰の水平移動はCharacterMovementが担当する。
                if (bone == 0)
                {
                    const FVector location = local.GetLocation();
                    local.SetLocation(FVector(0.0f, 0.0f, location.Z));
                }
                pose *= local;
            }
            positions.Add(pose.GetLocation());
            low = FMath::Min(low, pose.GetLocation().Z);
            high = FMath::Max(high, pose.GetLocation().Z);
        }
        //遊脚の速い振り戻しを除き、低い位置を通る足の速度だけを集める。
        const float ground = low + (high - low) * 0.25f;
        for (int32 sample = 1; sample < samples; ++sample)
        {
            if (positions[sample].Z > ground || positions[sample - 1].Z > ground) { continue; }
            const float speed = FVector::Dist2D(positions[sample], positions[sample - 1]) / step;
            if (speed > 10.0f) { speeds.Add(speed); }
        }
    }
    if (speeds.Num() < 4) { return _fallback; }
    //接地直前の揺れに引っ張られないよう、平均ではなく中央の速度を使う。
    speeds.Sort();
    return FMath::Clamp(speeds[speeds.Num() / 2], 40.0f, 900.0f);
}
