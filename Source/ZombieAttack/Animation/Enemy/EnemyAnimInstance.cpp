#include "EnemyAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "AnimNodes/AnimNode_Slot.h"
#include "ZombieAttack/Enemy/EnemyChara.h"

//ゲームスレッドで取得した速度だけを使い、並列評価中にActorへアクセスしない
struct FEnemyAnimProxy : FAnimInstanceProxy
{
    //停止、歩行、走行それぞれの再生位置を個体ごとに持つ
    FAnimNode_SequencePlayer_Standalone m_idle;
    FAnimNode_SequencePlayer_Standalone m_walk;
    FAnimNode_SequencePlayer_Standalone m_run;
    //歩行から走行、停止から移動の順に姿勢を混ぜる
    FAnimNode_TwoWayBlend m_gait;
    FAnimNode_TwoWayBlend m_movement;
    //攻撃と咆哮は移動姿勢の上から全身へ反映する
    FAnimNode_Slot m_action;
    //足の接地感を保ちながら停止と発進を短時間で補間する
    float m_moveWeight = 0.0f;

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
        }
        m_idle.SetLoopAnimation(true);
        m_walk.SetLoopAnimation(true);
        m_run.SetLoopAnimation(true);
        //歩行と走行の周期を揃え、速度変更時に左右の脚が逆位相で混ざるのを防ぐ。
        m_walk.SetGroupName(TEXT("EnemyGait"));
        m_run.SetGroupName(TEXT("EnemyGait"));
        m_walk.SetGroupMethod(EAnimSyncMethod::SyncGroup);
        m_run.SetGroupMethod(EAnimSyncMethod::SyncGroup);
        m_gait.A.SetLinkNode(&m_walk);
        m_gait.B.SetLinkNode(&m_run);
        m_movement.A.SetLinkNode(&m_idle);
        m_movement.B.SetLinkNode(&m_gait);
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
        m_moveWeight = FMath::FInterpTo(m_moveWeight, speed > 4.0f ? 1.0f : 0.0f, _deltaSeconds, 14.0f);
        m_movement.Alpha = m_moveWeight;
        m_gait.Alpha = FMath::Clamp((speed - 150.0f) / 170.0f, 0.0f, 1.0f);
        m_walk.SetPlayRate(FMath::Clamp(speed / 125.0f, 0.35f, 2.0f));
        m_run.SetPlayRate(FMath::Clamp(speed / 500.0f, 0.45f, 1.3f));
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
