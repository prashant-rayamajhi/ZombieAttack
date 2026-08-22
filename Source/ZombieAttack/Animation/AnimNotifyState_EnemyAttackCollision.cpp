#include "AnimNotifyState_EnemyAttackCollision.h"

#include "ZombieAttack/Enemy/EnemyChara.h"

//アニメーション通知が開始されたときに呼び出される関数
void UAnimNotifyState_EnemyAttackCollision::NotifyBegin(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation, float _totalDuration,
                                                        const FAnimNotifyEventReference& _eventReference)
{
    //親クラスのNotifyBegin関数を呼び出す
    Super::NotifyBegin(_meshComp, _animation, _totalDuration, _eventReference);

    //メッシュコンポーネントが有効でない場合は処理を終了
    if (!_meshComp) { return; }

    //メッシュコンポーネントの所有者をAEnemyChara型にキャスト
    AEnemyChara* enemy = Cast<AEnemyChara>(_meshComp->GetOwner());
    //「!enemy」が成立するとき、ResetAttackHitForNewSwingを呼び出します。
    if (!enemy) { return; }

    //敵の攻撃ヒット処理をリセットし、新しいスイングに備える
    enemy->ResetAttackHitForNewSwing();
    enemy->SetAttackCollisionEnabled(true);
    enemy->BeginAttackVFXWindow();
}

//アニメーション通知が終了したときに呼び出される関数
void UAnimNotifyState_EnemyAttackCollision::NotifyEnd(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation,
                                                      const FAnimNotifyEventReference& _eventReference)
{
    //親クラスのNotifyEnd関数を呼び出す
    Super::NotifyEnd(_meshComp, _animation, _eventReference);

    //メッシュコンポーネントが有効でない場合は処理を終了
    if (!_meshComp) { return; }

    //メッシュコンポーネントの所有者をAEnemyChara型にキャスト
    AEnemyChara* enemy = Cast<AEnemyChara>(_meshComp->GetOwner());
    //「!enemy」が成立するとき、SetAttackCollisionEnabledを呼び出します。
    if (!enemy) { return; }

    //敵の攻撃コリジョンを無効化する
    enemy->SetAttackCollisionEnabled(false);
    enemy->EndAttackVFXWindow();
}
