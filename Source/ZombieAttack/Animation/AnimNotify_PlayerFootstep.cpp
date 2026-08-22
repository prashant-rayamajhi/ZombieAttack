#include "AnimNotify_PlayerFootstep.h"

#include "../Components/PlayerAudio/PlayerAudioComponent.h"
#include "Components/SkeletalMeshComponent.h"

//Montage上の通知位置で、対応するゲーム処理を所有者へ伝えます。
void UAnimNotify_PlayerFootstep::Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation,
                                        const FAnimNotifyEventReference& _eventReference)
{
    Super::Notify(_meshComp, _animation, _eventReference);

    //所有者を返します。
    AActor* owner = _meshComp ? _meshComp->GetOwner() : nullptr;
    //「!owner」が成立するとき、続けて「UPlayerAudioComponent* audioComponent = owner->FindComponentByClass<U…」を判定します。
    if (!owner) { return; }

    //「UPlayerAudioComponent* audioComponent = owner->FindComponentByClass<UPlayerAudioComponent…」が成立するとき、PlayFootstepを呼び出します。
    if (UPlayerAudioComponent* audioComponent = owner->FindComponentByClass<UPlayerAudioComponent>())
    {
        audioComponent->PlayFootstep();
    }
}
