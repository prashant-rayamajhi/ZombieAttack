#include "AnimNotify_RifleAimReady.h"

#include "../Player/PlayerChara.h"
#include "Components/SkeletalMeshComponent.h"

//Montage上の通知位置で、対応するゲーム処理を所有者へ伝えます。
void UAnimNotify_RifleAimReady::Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation,
                                       const FAnimNotifyEventReference& _eventReference)
{
    Super::Notify(_meshComp, _animation, _eventReference);
    if (!_meshComp) { return; }
    if (APlayerChara* player = Cast<APlayerChara>(_meshComp->GetOwner()))
    {
        player->FinishRifleAimTransitionFromAnimation();
    }
}
