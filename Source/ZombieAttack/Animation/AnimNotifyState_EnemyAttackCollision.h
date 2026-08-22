#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_EnemyAttackCollision.generated.h"

//AnimNotify状態敵攻撃Collisionの動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API UAnimNotifyState_EnemyAttackCollision : public UAnimNotifyState
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //アニメーション通知が開始されたときに呼び出される関数
    virtual void NotifyBegin(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation, float _totalDuration,
                             //overrideをゲーム処理から参照できるように管理します。
                             const FAnimNotifyEventReference& _eventReference) override;

    //アニメーション通知が更新されたときに呼び出される関数
    virtual void NotifyEnd(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation,
                           //overrideをゲーム処理から参照できるように管理します。
                           const FAnimNotifyEventReference& _eventReference) override;
};
