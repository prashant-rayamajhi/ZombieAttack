#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyAttackTraceComponent.generated.h"

//骨の更新後に手足の通過範囲を調べ、速い攻撃のすり抜けを防ぐ。
UCLASS()
class ZOMBIEATTACK_API UEnemyAttackTraceComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    //アニメーション評価後に判定する実行順を設定する。
    UEnemyAttackTraceComponent();
protected:
    //所有する敵の骨更新が終わるまで接触判定を待たせる。
    virtual void BeginPlay() override;
    //通知で開いている攻撃区間だけ手足の移動を調べる。
    virtual void TickComponent(float _deltaTime, ELevelTick _tickType, FActorComponentTickFunction* _tickFunction) override;
};
