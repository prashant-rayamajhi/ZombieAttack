#include "AnimNotify_RifleAimReady.h"

#include "../Player/PlayerChara.h"
#include "Components/SkeletalMeshComponent.h"

//Montage上の通知位置で、対応するゲーム処理を所有者へ伝えます。
void UAnimNotify_RifleAimReady::Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation,
                                       const FAnimNotifyEventReference& _eventReference)
{
    Super::Notify(_meshComp, _animation, _eventReference);

    //「!_meshComp」が成立するとき、続けて「APlayerChara* player = Cast<APlayerChara>(_meshComp->GetOwner())」を判定します。
    if (!_meshComp) { return; }

    //「APlayerChara* player = Cast<APlayerChara>(_meshComp->GetOwner())」が成立するとき、FinishRifleAimTransitionFromAnimationを呼び出します。
    if (APlayerChara* player = Cast<APlayerChara>(_meshComp->GetOwner()))
    {
        player->FinishRifleAimTransitionFromAnimation();
    }
}
