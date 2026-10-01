#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "ZombieAttack/Components/Combat/AttackInputBuffer.h"
#include "ZombieAttack/Player/PlayerChara.h"
#include "ZombieAttack/Weapon/GunWeapon.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"

//短い先行入力だけを受け付け、連打や装備変更で予約が暴発しないことを確認する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackBufferTest, "ZombieAttack.Player.AttackBuffer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAttackBufferTest::RunTest(const FString& _parameters)
{
    FAttackInputBuffer buffer;
    TestFalse(TEXT("empty buffer cannot fire"), buffer.Consume(0, 0));
    buffer.Queue(0, 1.0);
    TestTrue(TEXT("near completion press is accepted"), buffer.Consume(0, 1.17));
    TestFalse(TEXT("press cannot repeat"), buffer.Consume(0, 1.17));
    buffer.Queue(0, 2.0);
    TestFalse(TEXT("old press expires"), buffer.Consume(0, 2.19));
    buffer.Queue(0, 3.0);
    TestFalse(TEXT("different weapon cannot inherit press"), buffer.Consume(1, 3.1));
    buffer.Queue(1, 4.0);
    buffer.Clear();
    TestFalse(TEXT("released rifle cannot fire"), buffer.Consume(1, 4.1));
    buffer.Queue(0, 5.0);
    buffer.Queue(0, 5.1);
    TestTrue(TEXT("latest press replaces previous"), buffer.Consume(0, 5.2));
    TestFalse(TEXT("multiple presses do not stack"), buffer.Consume(0, 5.2));
    return true;
}

//連続撃破で時間倍率がさらに下がらず、画面遷移で元の倍率へ戻ることを確認する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKillStopTest, "ZombieAttack.Player.KillStopRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKillStopTest::RunTest(const FString& _parameters)
{
    UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
    APlayerChara* player = world->SpawnActor<APlayerChara>();
    UGameplayStatics::SetGlobalTimeDilation(world, 0.5f);
    player->PlayKillHitStop();
    const float stopped = UGameplayStatics::GetGlobalTimeDilation(world);
    TestTrue(TEXT("kill briefly slows world"), stopped < 0.5f);
    player->PlayKillHitStop();
    TestEqual(TEXT("consecutive kill does not compound stop"), UGameplayStatics::GetGlobalTimeDilation(world), stopped);
    //時間倍率を掛けたフレーム時間を渡しても、カメラ入力だけは撃破前の速度で反映される。
    auto* input = NewObject<UInputComponent>(player);
    player->SetupPlayerInputComponent(input);
    auto* arm = player->FindComponentByClass<USpringArmComponent>();
    const float beforeYaw = arm->GetRelativeRotation().Yaw;
    for (FInputAxisBinding& binding : input->AxisBindings)
    {
        if (binding.AxisName == TEXT("CameraYaw")) { binding.AxisDelegate.Execute(1.0f); }
    }
    player->Tick(stopped / 60.0f);
    TestTrue(TEXT("camera retains pre-stop input speed"), FMath::IsNearlyEqual(arm->GetRelativeRotation().Yaw - beforeYaw, 0.5f, 0.01f));
    player->EndPlay(EEndPlayReason::RemovedFromWorld);
    TestEqual(TEXT("cleanup restores prior slow motion"), UGameplayStatics::GetGlobalTimeDilation(world), 0.5f);
    GEngine->DestroyWorldContext(world);
    world->DestroyWorld(false);
    return true;
}
//実際のピストルBPで、同じフレームの連打が追加弾を消費しないことを確認する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPistolCadenceTest, "ZombieAttack.Weapons.PistolCadence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPistolCadenceTest::RunTest(const FString& _parameters)
{
    UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
    APlayerChara* player = world->SpawnActor<APlayerChara>();
    APlayerController* controller = world->SpawnActor<APlayerController>();
    controller->Possess(player);
    UClass* cls = LoadClass<AGunWeapon>(nullptr, TEXT("/Game/Blueprints/Weapons/BP_Pistol.BP_Pistol_C"));
    AGunWeapon* gun = world->SpawnActor<AGunWeapon>(cls);
    if (TestNotNull(TEXT("pistol asset loaded"), gun))
    {
        gun->SetOwnerCharacter(player);
        const int32 before = gun->GetCurrentAmmo();
        gun->UseWeapon();
        TestEqual(TEXT("first press fires once"), gun->GetCurrentAmmo(), before - 1);
        gun->UseWeapon();
        TestEqual(TEXT("same frame press cannot fire again"), gun->GetCurrentAmmo(), before - 1);
        TestFalse(TEXT("cooldown blocks new shot"), gun->CanFireNow());
        for (int32 frame = 0; frame < 8; ++frame) { world->Tick(LEVELTICK_All, 0.1f); }
        TestTrue(TEXT("cooldown eventually releases"), gun->CanFireNow());
    }
    GEngine->DestroyWorldContext(world);
    world->DestroyWorld(false);
    return true;
}
#endif
