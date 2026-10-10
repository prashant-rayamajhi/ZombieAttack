#include "EnemyAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "AnimNodes/AnimNode_Slot.h"
#include "ZombieAttack/Enemy/EnemyChara.h"
#include "EnemyStride.h"
#include "Components/SkeletalMeshComponent.h"

//ゲームスレッドで取得した速度だけを使い、並列評価中にActorへアクセスしない
struct FEnemyAnimProxy : FAnimInstanceProxy
{
    //停止、歩行、走行それぞれの再生位置を個体ごとに持つ
    FAnimNode_SequencePlayer_Standalone m_idle;
    FAnimNode_SequencePlayer_Standalone m_walk;
    FAnimNode_SequencePlayer_Standalone m_run;
    //横移動用の左右クリップを独立してループ再生する。
    FAnimNode_SequencePlayer_Standalone m_left;
    FAnimNode_SequencePlayer_Standalone m_right;
    //左右の選択と、前後移動から横移動への移行を滑らかにつなぐ。
    FAnimNode_TwoWayBlend m_side;
    FAnimNode_TwoWayBlend m_direction;
    //横移動一周期の歩幅を、体格ごとに持つ。
    float m_leftStride = 150.0f;
    float m_rightStride = 150.0f;
    //歩行から走行、停止から移動の順に姿勢を混ぜる
    FAnimNode_TwoWayBlend m_gait;
    FAnimNode_TwoWayBlend m_movement;
    //攻撃と咆哮は移動姿勢の上から全身へ反映する
    FAnimNode_Slot m_action;
    //足の接地感を保ちながら停止と発進を短時間で補間する
    float m_moveWeight = 0.0f;
    //各クリップの一周期で進む距離。同期グループが再生速度を上書きしても歩幅を揃える。
    float m_walkStride = 125.0f;
    float m_runStride = 500.0f;

    explicit FEnemyAnimProxy(UAnimInstance* _instance) : FAnimInstanceProxy(_instance) {}

    //各敵に設定された同一Skeletonの移動アニメーションを接続する
    virtual void Initialize(UAnimInstance* _instance) override
    {
        FAnimInstanceProxy::Initialize(_instance);
        const AEnemyChara* enemy = Cast<AEnemyChara>(_instance->TryGetPawnOwner());
        if (enemy)
        {
            m_idle.SetSequence(enemy->GetIdleAnimation());
            m_walk.SetSequence(enemy->GetWalkAnimation());
            m_run.SetSequence(enemy->GetRunAnimation());
            m_left.SetSequence(enemy->GetStrafeLeftAnimation());
            m_right.SetSequence(enemy->GetStrafeRightAnimation());
            const float scale = enemy->GetMesh()->GetComponentScale().GetAbsMax();
            if (const UAnimSequence* clip = enemy->GetWalkAnimation())
            {
                m_walkStride = EnemyStride::MeasureSpeed(clip, 125.0f) * clip->GetPlayLength() * scale;
            }
            if (const UAnimSequence* clip = enemy->GetRunAnimation())
            {
                m_runStride = EnemyStride::MeasureSpeed(clip, 500.0f) * clip->GetPlayLength() * scale;
            }
            if (const UAnimSequence* clip = enemy->GetStrafeLeftAnimation())
            {
                m_leftStride = EnemyStride::MeasureSpeed(clip, 180.0f) * clip->GetPlayLength() * scale;
            }
            if (const UAnimSequence* clip = enemy->GetStrafeRightAnimation())
            {
                m_rightStride = EnemyStride::MeasureSpeed(clip, 180.0f) * clip->GetPlayLength() * scale;
            }
        }
        m_idle.SetLoopAnimation(true);
        m_walk.SetLoopAnimation(true);
        m_run.SetLoopAnimation(true);
        for (FAnimNode_SequencePlayer_Standalone* clip : {&m_left, &m_right})
        {
            clip->SetLoopAnimation(true);
            clip->SetGroupName(TEXT("EnemyGait"));
            clip->SetGroupMethod(EAnimSyncMethod::SyncGroup);
        }
        //歩行と走行の周期を揃え、速度変更時に左右の脚が逆位相で混ざるのを防ぐ。
        m_walk.SetGroupName(TEXT("EnemyGait"));
        m_run.SetGroupName(TEXT("EnemyGait"));
        m_walk.SetGroupMethod(EAnimSyncMethod::SyncGroup);
        m_run.SetGroupMethod(EAnimSyncMethod::SyncGroup);
        m_gait.A.SetLinkNode(&m_walk);
        m_gait.B.SetLinkNode(&m_run);
        m_movement.A.SetLinkNode(&m_idle);
        m_side.A.SetLinkNode(&m_left);
        m_side.B.SetLinkNode(&m_right);
        m_direction.A.SetLinkNode(&m_gait);
        m_direction.B.SetLinkNode(&m_side);
        m_movement.B.SetLinkNode(&m_direction);
        m_action.Source.SetLinkNode(&m_movement);
        m_action.SlotName = TEXT("DefaultSlot");
        m_action.bAlwaysUpdateSourcePose = true;
        m_action.Initialize_AnyThread(FAnimationInitializeContext(this));
    }

    //移動速度をクリップの基準速度へ合わせ、Idleのまま滑る状態を防ぐ
    virtual void PreUpdate(UAnimInstance* _instance, float _deltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(_instance, _deltaSeconds);
        const APawn* pawn = _instance->TryGetPawnOwner();
        const float speed = pawn ? pawn->GetVelocity().Size2D() : 0.0f;
        //Actorの正面と実速度の角度から、左右の回り込みを選択する。
        const FVector velocity = pawn ? pawn->GetVelocity().GetSafeNormal2D() : FVector::ZeroVector;
        const float side = pawn ? FVector::DotProduct(velocity, pawn->GetActorRightVector()) : 0.0f;
        const bool hasSideClips = m_left.GetSequence() && m_right.GetSequence();
        m_side.Alpha = side >= 0.0f ? 1.0f : 0.0f;
        m_direction.Alpha = FMath::FInterpTo(m_direction.Alpha, hasSideClips ? FMath::Abs(side) : 0.0f, _deltaSeconds, 12.0f);
        m_moveWeight = FMath::FInterpTo(m_moveWeight, speed > 4.0f ? 1.0f : 0.0f, _deltaSeconds, 14.0f);
        m_movement.Alpha = m_moveWeight;
        m_gait.Alpha = FMath::Clamp((speed - 150.0f) / 170.0f, 0.0f, 1.0f);
        //歩行と走行を混ぜた歩幅に移動距離を合わせ、両クリップを同じ周期で進める。
        const float forwardStride = FMath::Lerp(m_walkStride, m_runStride, m_gait.Alpha);
        const float sideStride = FMath::Lerp(m_leftStride, m_rightStride, m_side.Alpha);
        const float stride = FMath::Max(1.0f, FMath::Lerp(forwardStride, sideStride, m_direction.Alpha));
        const float cycles = speed / stride;
        if (const UAnimSequenceBase* clip = m_walk.GetSequence())
        {
            m_walk.SetPlayRate(cycles * clip->GetPlayLength() / FMath::Max(0.01f, FMath::Abs(clip->RateScale)));
        }
        if (const UAnimSequenceBase* clip = m_run.GetSequence())
        {
            m_run.SetPlayRate(cycles * clip->GetPlayLength() / FMath::Max(0.01f, FMath::Abs(clip->RateScale)));
        }
        for (FAnimNode_SequencePlayer_Standalone* node : {&m_left, &m_right})
        {
            if (const UAnimSequenceBase* clip = node->GetSequence())
            {
                node->SetPlayRate(cycles * clip->GetPlayLength() / FMath::Max(0.01f, FMath::Abs(clip->RateScale)));
            }
        }
    }

    //メッシュのLOD変更時も全身スロットのボーン参照を更新する
    virtual void CacheBones() override { m_action.CacheBones_AnyThread(FAnimationCacheBonesContext(this)); }
    //スロットを更新してモンタージュの重みと通知をエンジンへ渡す
    virtual void UpdateAnimationNode(const FAnimationUpdateContext& _context) override { m_action.Update_AnyThread(_context); }
    //共通の移動姿勢と現在の全身アクションから最終姿勢を作る
    virtual bool Evaluate(FPoseContext& _output) override
    {
        m_action.Evaluate_AnyThread(_output);
        //Mixamoの最上位骨は腰なので、回転と上下動を固定するとパンチや屈伸が崩れる。
        //カプセルから大きく離れる水平移動だけを抑え、踏み込みと腰のひねりは残す。
        FTransform& root = _output.Pose[FCompactPoseBoneIndex(0)];
        const FVector origin = _output.Pose.GetBoneContainer().GetRefPoseArray()[0].GetLocation();
        const FVector location = root.GetLocation();
        const FVector sway = FVector(location.X - origin.X, location.Y - origin.Y, 0.0f).GetClampedToMaxSize(35.0f);
        root.SetLocation(FVector(origin.X + sway.X, origin.Y + sway.Y, location.Z));
        return true;
    }
};

FAnimInstanceProxy* UEnemyAnimInstance::CreateAnimInstanceProxy() { return new FEnemyAnimProxy(this); }
void UEnemyAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* _proxy) { delete _proxy; }
