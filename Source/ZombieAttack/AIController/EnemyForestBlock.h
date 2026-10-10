#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyForestBlock.generated.h"

class UNavModifierComponent;

//森の内部を移動経路から除き、プレイヤー用の物理的な壁は作らない。
UCLASS()
class AEnemyForestBlock : public AActor
{
    GENERATED_BODY()
public:
    AEnemyForestBlock();
    //エディタで範囲を変更した時も、経路探索の禁止領域へ反映する。
    virtual void OnConstruction(const FTransform& _transform) override;
    //通常経路を使わない突進も、禁止領域に入る手前で止める。
    static bool BlocksStep(const UWorld* _world, const FVector& _start, const FVector& _end, float _radius);
    //地面の起伏ごと覆う禁止範囲の半サイズ。
    UPROPERTY(EditAnywhere, Category = "Navigation")
    FVector m_extent = FVector(100.0f, 100.0f, 5000.0f);
private:
    //歩行可能なポリゴンを森の内部だけ取り除く。
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UNavModifierComponent> m_modifier;
};
