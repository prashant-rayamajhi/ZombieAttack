#include "EnemyForestBlock.h"
#include "Components/SceneComponent.h"
#include "NavModifierComponent.h"
#include "NavAreas/NavArea_Null.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"

AEnemyForestBlock::AEnemyForestBlock()
{
    //見えない壁としてプレイヤーを止めず、敵の経路だけを制限する。
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    m_modifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("ForestNavigation"));
    m_modifier->SetAreaClass(UNavArea_Null::StaticClass());
    m_modifier->FailsafeExtent = m_extent;
}

void AEnemyForestBlock::OnConstruction(const FTransform& _transform)
{
    Super::OnConstruction(_transform);
    m_modifier->FailsafeExtent = m_extent;
    m_modifier->CalcAndCacheBounds();
    m_modifier->RefreshNavigationModifiers();
}

bool AEnemyForestBlock::BlocksStep(const UWorld* _world, const FVector& _start, const FVector& _end, float _radius)
{
    if (!_world) { return false; }
    //線分の両端だけでなく途中も判定し、低フレームレートで境界を飛び越えない。
    for (TActorIterator<AEnemyForestBlock> block(_world); block; ++block)
    {
        const FVector extent = block->m_extent + FVector(_radius, _radius, 0.0f);
        const FBox bounds(block->GetActorLocation() - extent, block->GetActorLocation() + extent);
        //境界内から押し出された敵は道へ戻れるよう、外へ出る方向だけ許可する。
        if (bounds.IsInside(_start) && !bounds.IsInside(_end)) { continue; }
        if (FMath::LineBoxIntersection(bounds, _start, _end, _end - _start)) { return true; }
    }
    return false;
}
