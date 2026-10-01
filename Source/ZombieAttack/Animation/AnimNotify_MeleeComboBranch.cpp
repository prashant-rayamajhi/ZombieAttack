#include "AnimNotify_MeleeComboBranch.h"

#include "ZombieAttack/Player/PlayerChara.h"
#include "ZombieAttack/Weapon/MeleeWeapon.h"

//アニメーション通知が発生したときに呼び出される関数
void UAnimNotify_MeleeComboBranch::Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation,
                                          const FAnimNotifyEventReference& _eventReference)
{
    //親クラスのNotify関数を呼び出す
    Super::Notify(_meshComp, _animation, _eventReference);

    //メッシュコンポーネントが有効でない場合は処理を終了
    if (!_meshComp) { return; }

    //メッシュコンポーネントの所有者をAPlayerChara型にキャスト
    APlayerChara* player = Cast<APlayerChara>(_meshComp->GetOwner());
    if (!player) { return; }

    //プレイヤーの現在の武器をAMeleeWeapon型にキャスト
    AMeleeWeapon* meleeWeapon = Cast<AMeleeWeapon>(player->GetCurrentWeapon());
    if (!meleeWeapon) { return; }

    //メレー武器のコンボ継続処理を実行
    meleeWeapon->TryContinueComboFromNotify();
}
