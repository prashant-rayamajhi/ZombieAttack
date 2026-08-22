#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EnemyComboBranch.generated.h"

//AnimNotify敵ComboBranchの動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API UAnimNotify_EnemyComboBranch : public UAnimNotify
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //アニメーション通知が発生したときに呼び出される関数
    virtual void Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation,
                        //overrideをゲーム処理から参照できるように管理します。
                        const FAnimNotifyEventReference& _eventReference) override;
};
