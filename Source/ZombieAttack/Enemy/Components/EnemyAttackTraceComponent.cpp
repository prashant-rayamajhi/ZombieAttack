#include "EnemyAttackTraceComponent.h"
#include "../EnemyChara.h"
#include "Components/SkeletalMeshComponent.h"

UEnemyAttackTraceComponent::UEnemyAttackTraceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UEnemyAttackTraceComponent::BeginPlay()
{
    Super::BeginPlay();
    if (AEnemyChara* enemy = Cast<AEnemyChara>(GetOwner())) { AddTickPrerequisiteComponent(enemy->GetMesh()); }
}

void UEnemyAttackTraceComponent::TickComponent(float _deltaTime, ELevelTick _tickType, FActorComponentTickFunction* _tickFunction)
{
    Super::TickComponent(_deltaTime, _tickType, _tickFunction);
    if (AEnemyChara* enemy = Cast<AEnemyChara>(GetOwner())) { enemy->TraceAttackContact(); }
}
