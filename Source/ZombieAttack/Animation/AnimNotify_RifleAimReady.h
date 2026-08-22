#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_RifleAimReady.generated.h"

//Rifle Down To Aimの終端からPlayerへ構え完了を通知します。
//発射許可とAiming Idleへの遷移を、見た目の完了フレームへ同期させます。
UCLASS()
class ZOMBIEATTACK_API UAnimNotify_RifleAimReady : public UAnimNotify
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    virtual void Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation,
                        //overrideをゲーム処理から参照できるように管理します。
                        const FAnimNotifyEventReference& _eventReference) override;
};
