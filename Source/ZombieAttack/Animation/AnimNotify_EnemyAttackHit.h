#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EnemyAttackHit.generated.h"

//AnimNotify敵攻撃Hitの動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API UAnimNotify_EnemyAttackHit : public UAnimNotify
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //アニメーション通知が発生したときに呼び出される関数
    virtual void Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation,
                        //overrideをゲーム処理から参照できるように管理します。
                        const FAnimNotifyEventReference& _eventReference) override;

  private:
    //攻撃のダメージ倍率を設定するプロパティ
    UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMin = "0.0"))
    float m_damageMultiplier = 1.0f;
};
