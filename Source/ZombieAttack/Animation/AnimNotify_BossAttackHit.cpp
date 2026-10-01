#include "AnimNotify_BossAttackHit.h"
#include "ZombieAttack/Enemy/BossChara/BossChara.h"

//アニメーション通知が発生したときに呼び出される関数
void UAnimNotify_BossAttackHit::Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation,
                                       const FAnimNotifyEventReference& _eventReference)
{
    //親クラスのNotify関数を呼び出す
    Super::Notify(_meshComp, _animation, _eventReference);

    //メッシュコンポーネントが有効でない場合は処理を終了
    if (!_meshComp) { return; }

    //メッシュコンポーネントの所有者をABossChara型にキャスト
    ABossChara* boss = Cast<ABossChara>(_meshComp->GetOwner());
    if (!boss) { return; }

    //ボスの攻撃ヒット処理を実行
    boss->PerformBossAttackHit();
}
