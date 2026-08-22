#include "AnimNotify_EnemyAttackHit.h"
#include "ZombieAttack/Enemy/EnemyChara.h"

//アニメーション通知が発生したときに呼び出される関数
void UAnimNotify_EnemyAttackHit::Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation,
                                        const FAnimNotifyEventReference& _eventReference)
{
    //親クラスのNotify関数を呼び出す
    Super::Notify(_meshComp, _animation, _eventReference);

    //メッシュコンポーネントが有効でない場合は処理を終了
    if (!_meshComp) { return; }

    //メッシュコンポーネントの所有者をAEnemyChara型にキャスト
    AEnemyChara* enemy = Cast<AEnemyChara>(_meshComp->GetOwner());
    //「!enemy」が成立するとき、PerformAttackHitを呼び出します。
    if (!enemy) { return; }

    //敵の攻撃ヒット処理を実行
    enemy->PerformAttackHit(m_damageMultiplier);
}
