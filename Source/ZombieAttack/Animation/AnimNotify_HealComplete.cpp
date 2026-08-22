#include "AnimNotify_HealComplete.h"
#include "ZombieAttack/Player/PlayerChara.h"

//アニメーション通知が発生したときに呼び出される関数
void UAnimNotify_HealComplete::Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation,
                                      const FAnimNotifyEventReference& _eventReference)
{
    //親クラスのNotify関数を呼び出す
    Super::Notify(_meshComp, _animation, _eventReference);

    //メッシュコンポーネントが有効でない場合は処理を終了
    if (!_meshComp) { return; }

    //メッシュコンポーネントの所有者をAPlayerChara型にキャスト
    APlayerChara* player = Cast<APlayerChara>(_meshComp->GetOwner());
    //「!player」が成立するとき、ApplyHealを呼び出します。
    if (!player) { return; }

    //プレイヤーの回復処理を実行
    player->ApplyHeal(m_healAmount);
}
