#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "ZombieAttack/Weapon/WeaponAimTrace.h"

//敵より手前の壁と、肩越しカメラから見えない銃口側の遮蔽物を検証する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponAimTest, "ZombieAttack.Weapons.MuzzleOcclusion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponAimTest::RunTest(const FString& _parameters)
{
    UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
    //Visibilityを無視する敵も検出できるよう、実際の敵と同じPawn判定を置く。
    const auto addBox = [world](const FVector& _position, const FVector& _extent, ECollisionChannel _type)
    {
        AActor* actor = world->SpawnActor<AActor>();
        auto* box = NewObject<UBoxComponent>(actor);
        actor->SetRootComponent(box);
        box->SetBoxExtent(_extent);
        box->SetCollisionObjectType(_type);
        box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        box->SetCollisionResponseToAllChannels(ECR_Block);
        if (_type == ECC_Pawn) { box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore); }
        box->RegisterComponent();
        actor->SetActorLocation(_position);
        return actor;
    };
    AActor* enemy = addBox(FVector(1000, 0, 0), FVector(40), ECC_Pawn);
    AActor* wall = addBox(FVector(500, 0, 0), FVector(20, 100, 100), ECC_WorldStatic);
    FHitResult hit;
    WeaponAimTrace::FindFirstHit(world, FVector::ZeroVector, FVector(1200, 0, 0), nullptr, nullptr, hit);
    TestEqual(TEXT("wall blocks enemy behind it"), hit.GetActor(), wall);
    wall->SetActorLocation(FVector(500, 300, 0));
    WeaponAimTrace::FindFirstHit(world, FVector::ZeroVector, FVector(1200, 0, 0), nullptr, nullptr, hit);
    TestEqual(TEXT("clear center ray reaches pawn"), hit.GetActor(), enemy);
    //カメラ側は開いていても、横へずれた銃口の正面に壁があれば壁へ命中する。
    wall->SetActorLocation(FVector(200, 210, 0));
    WeaponAimTrace::FindFirstHit(world, FVector::ZeroVector, FVector(1200, 0, 0), nullptr, nullptr, hit);
    TestEqual(TEXT("camera sees enemy beside wall"), hit.GetActor(), enemy);
    WeaponAimTrace::FindFirstHit(world, FVector(100, 260, 0), FVector(1000, 0, 0), nullptr, nullptr, hit);
    TestEqual(TEXT("offset muzzle hits wall first"), hit.GetActor(), wall);
    TestFalse(TEXT("empty sky has no hit"), WeaponAimTrace::FindFirstHit(world, FVector(0, 0, 500), FVector(1200, 0, 500),
        nullptr, nullptr, hit));
    GEngine->DestroyWorldContext(world);
    world->DestroyWorld(false);
    return true;
}
#endif
