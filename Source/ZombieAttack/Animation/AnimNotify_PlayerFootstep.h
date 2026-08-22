#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_PlayerFootstep.generated.h"

//接地フレームでPlayerAudioComponentへ足音再生を通知します。
UCLASS()
class ZOMBIEATTACK_API UAnimNotify_PlayerFootstep : public UAnimNotify
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    virtual void Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation,
                        //overrideをゲーム処理から参照できるように管理します。
                        const FAnimNotifyEventReference& _eventReference) override;
};
