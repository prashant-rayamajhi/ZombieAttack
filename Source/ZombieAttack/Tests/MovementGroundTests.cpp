#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Animation/BlendSpace.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "UObject/UnrealType.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "NiagaraComponent.h"
#include "UObject/UObjectIterator.h"
#include "ZombieAttack/Player/PlayerChara.h"
#include "ZombieAttack/Animation/Player/PlayerAnimInstance.h"
#include "ZombieAttack/Enemy/BossChara/BossChara.h"
#include "ZombieAttack/Goal/GoalActor.h"
#include "ZombieAttack/AIController/EnemyAIController.h"
#include "Materials/Material.h"

//カメラの向きが変わっても、S入力で体と視点が反転せず後退速度になることを検証する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerBackwardTest, "ZombieAttack.Player.BackwardMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerBackwardTest::RunTest(const FString& _parameters)
{
    UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
    UClass* playerClass = LoadClass<APlayerChara>(nullptr, TEXT("/Game/Blueprints/Player/Actor/BP_PlayerChara.BP_PlayerChara_C"));
    APlayerChara* player = world->SpawnActor<APlayerChara>(playerClass);
    UInputComponent* input = NewObject<UInputComponent>(player);
    player->SetupPlayerInputComponent(input);
    USpringArmComponent* arm = player->FindComponentByClass<USpringArmComponent>();
    UPlayerAnimInstance* animation = Cast<UPlayerAnimInstance>(player->GetMesh()->GetAnimInstance());
    TestNotNull(TEXT("existing AnimBP inherits signed locomotion"), animation);
    for (float yaw : {0.0f, 90.0f, -135.0f})
    {
        arm->SetRelativeRotation(FRotator(-10.0f, yaw, 0.0f));
        for (FInputAxisBinding& binding : input->AxisBindings)
        {
            if (binding.AxisName == TEXT("MoveForward")) { binding.AxisDelegate.Execute(-1.0f); }
        }
        for (int32 frame = 0; frame < 60; ++frame) { player->Tick(1.0f / 30.0f); }
        TestTrue(TEXT("backward input keeps camera yaw"), FMath::Abs(FMath::FindDeltaAngleDegrees(arm->GetComponentRotation().Yaw, yaw)) < 0.1f);
        const float bodyYaw = player->GetMesh()->GetComponentRotation().Yaw - player->GetBaseRotationOffsetRotator().Yaw;
        TestTrue(TEXT("backward input keeps body facing camera forward"), FMath::Abs(FMath::FindDeltaAngleDegrees(bodyYaw, yaw)) < 0.1f);
        player->GetCharacterMovement()->Velocity = -FRotator(0, yaw, 0).Vector() * 300.0f;
        if (animation)
        {
            animation->NativeUpdateAnimation(1.0f / 30.0f);
            TestTrue(TEXT("backward animation receives negative speed"), animation->m_locomotionSpeed < -299.0f);
        }
        TestTrue(TEXT("idle transitions retain positive speed"), player->GetMoveSpeed() > 299.0f);
    }
    //実際のAnimBPを数周進め、一度の再生で足が停止しないことを確認する。
    if (animation)
    {
        if (FObjectPropertyBase* reference = FindFProperty<FObjectPropertyBase>(animation->GetClass(), TEXT("PlayerRef")))
        {
            reference->SetObjectPropertyValue_InContainer(animation, player);
        }
        player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        float previousTime = 0.0f;
        int32 wraps = 0;
        bool activeBackward = false;
        for (int32 frame = 0; frame < 240; ++frame)
        {
            player->GetMesh()->TickAnimation(1.0f / 30.0f, false);
            player->GetMesh()->RefreshBoneTransforms();
            for (TFieldIterator<FStructProperty> property(animation->GetClass()); property; ++property)
            {
                if (property->Struct != FAnimNode_BlendSpacePlayer::StaticStruct()) { continue; }
                auto* node = property->ContainerPtrToValuePtr<FAnimNode_BlendSpacePlayer>(animation);
                if (!node->GetBlendSpace() || !node->GetBlendSpace()->GetName().Contains(TEXT("Pistol"))) { continue; }
                if (node->GetPosition().X >= -100.0f) { continue; }
                activeBackward = true;
                TestTrue(TEXT("backward player loops"), node->IsLooping());
                const float time = node->GetAccumulatedTime();
                if (time < previousTime) { ++wraps; }
                previousTime = time;
            }
        }
        TestTrue(TEXT("runtime graph selects backward locomotion"), activeBackward);
        TestTrue(TEXT("backward animation completes multiple cycles"), wraps >= 3);
    }
    for (const TCHAR* weapon : {TEXT("Knife"), TEXT("Pistol"), TEXT("AR")})
    {
        const FString path = FString::Printf(TEXT("/Game/Assets/Player/Animation/BlendSpace/BS_%s_Locomotion"), weapon);
        UBlendSpace* blend = LoadObject<UBlendSpace>(nullptr, *path);
        if (!TestNotNull(path, blend)) { continue; }
        bool backward = false;
        for (const FBlendSample& sample : blend->GetBlendSamples())
        {
            if (sample.SampleValue.X < -100.0f)
            {
                backward = true;
                TestNotNull(TEXT("backward sample has animation"), sample.Animation.Get());
            }
        }
        TestTrue(path + TEXT(" has backward samples"), backward);
    }
    GEngine->DestroyWorldContext(world);
    world->DestroyWorld(false);
    return true;
}

//ボスの衝撃波の生成原点と、出口の素材参照が実行時にも設定されることを確認する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundEffectsTest, "ZombieAttack.World.GroundEffects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGroundEffectsTest::RunTest(const FString& _parameters)
{
    UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
    world->SetGameInstance(NewObject<UGameInstance>(GEngine));
    AActor* floor = world->SpawnActor<AActor>();
    UBoxComponent* box = NewObject<UBoxComponent>(floor);
    floor->SetRootComponent(box);
    box->SetBoxExtent(FVector(3000, 3000, 10));
    box->SetCollisionObjectType(ECC_WorldStatic);
    box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    box->SetCollisionResponseToAllChannels(ECR_Block);
    box->RegisterComponent();
    floor->SetActorLocation(FVector(0, 0, -10));
    FActorSpawnParameters spawn;
    spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    APlayerChara* player = world->SpawnActor<APlayerChara>(APlayerChara::StaticClass(), FVector(120, 0, 100), FRotator::ZeroRotator, spawn);
    for (const TCHAR* name : {TEXT("BP_MidBossChara"), TEXT("BP_FinalBossChara")})
    {
        const FString path = FString::Printf(TEXT("/Game/Blueprints/Enemy/Actors/%s.%s_C"), name, name);
        UClass* cls = LoadClass<ABossChara>(nullptr, *path);
        ABossChara* boss = world->SpawnActor<ABossChara>(cls, FVector::ZeroVector, FRotator::ZeroRotator, spawn);
        boss->SetActorLocation(FVector(0, 0, boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
        boss->DispatchBeginPlay();
        boss->SpawnDefaultController();
        if (AEnemyAIController* controller = Cast<AEnemyAIController>(boss->GetController())) { controller->StopPatrol(); }
        player->SetActorLocation(boss->GetActorLocation() + FVector(120, 0, 0));
        boss->SetCombatTarget(player);
        TSet<UNiagaraComponent*> previous;
        for (TObjectIterator<UNiagaraComponent> it; it; ++it) { previous.Add(*it); }
        TestTrue(TEXT("boss attack starts in reach"), boss->RequestSpecificAttack(EBossAttackPattern::LightCombo, player->GetActorLocation()));
        boss->BeginAttackVFXWindow();
        int32 effects = 0;
        for (TObjectIterator<UNiagaraComponent> it; it; ++it)
        {
            if (previous.Contains(*it) || it->GetWorld() != world) { continue; }
            ++effects;
            TestTrue(TEXT("effect centered below boss"), it->GetComponentLocation().Size2D() < 1.0f);
            TestTrue(TEXT("effect on floor, not torso"), FMath::Abs(it->GetComponentLocation().Z - 3.0f) < 1.0f);
        }
        if (FApp::CanEverRender()) { TestTrue(TEXT("attack creates ground effect"), effects > 0); }
        boss->Destroy();
    }
    AGoalActor* goal = world->SpawnActor<AGoalActor>();
    TArray<UStaticMeshComponent*> parts;
    goal->GetComponents(parts);
    int32 textured = 0;
    for (UStaticMeshComponent* part : parts)
    {
        if (!part->GetName().StartsWith(TEXT("Beacon"))) { continue; }
        TestNotNull(TEXT("beacon material assigned"), part->GetMaterial(0));
        if (part->GetMaterial(0))
        {
            TestEqual(TEXT("beacon uses textured metal"), part->GetMaterial(0)->GetBaseMaterial()->GetName(), FString(TEXT("M_EvacBeacon")));
        }
        ++textured;
    }
    TestEqual(TEXT("all three beacon parts checked"), textured, 3);
    GEngine->DestroyWorldContext(world);
    world->DestroyWorld(false);
    return true;
}
#endif
