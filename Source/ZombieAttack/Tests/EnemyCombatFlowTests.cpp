#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "ZombieAttack/Enemy/EnemyChara.h"
#include "ZombieAttack/Player/PlayerChara.h"
#include "ZombieAttack/AIController/EnemyAIController.h"
#include "ZombieAttack/Enemy/BossChara/BossChara.h"

//TimerManagerは同じ描画フレームを二度進めないため、戦闘の検証も実フレームに分ける。
class FEnemyCombatFlowCommand : public IAutomationLatentCommand
{
    //検証結果を書き込む元のテスト。
    FAutomationTestBase* m_test;
    //戦闘だけを動かす検証専用ワールド。
    UWorld* m_world;
    //発見から攻撃と死亡までの遷移を確認する敵。
    AEnemyChara* m_enemy;
    //命中ごとの体力変化を確認するプレイヤー。
    APlayerChara* m_player;
    //咆哮を最後まで進めるための経過フレーム数。
    int32 m_frame = 0;
public:
    //検証中に必要なワールドと戦闘参加者を受け取る。
    FEnemyCombatFlowCommand(FAutomationTestBase* _test, UWorld* _world, AEnemyChara* _enemy, APlayerChara* _player)
        : m_test(_test), m_world(_world), m_enemy(_enemy), m_player(_player) {}

    //毎フレーム攻撃を進め、最後にワールドを破棄して他のテストへ影響を残さない。
    virtual bool Update() override
    {
        m_world->Tick(LEVELTICK_All, 1.0f / 30.0f);
        m_enemy->GetMesh()->TickAnimation(1.0f / 30.0f, false);
        m_enemy->GetMesh()->RefreshBoneTransforms();
        if (++m_frame < 100) { return false; }
        AEnemyAIController* controller = CastChecked<AEnemyAIController>(m_enemy->GetController());
        m_test->AddInfo(FString::Printf(TEXT("Flow state=%d target=%s cachedPlayer=%s sight=%d reach=%d enemy=%s player=%s hp=%.1f"),
            static_cast<int32>(controller->GetCurrentState()), *GetNameSafe(controller->GetSensedActor()),
            *GetNameSafe(m_enemy->GetCombatTarget()), controller->LineOfSightTo(m_player), m_enemy->CanHitPlayer(150.0f, -1.0f),
            *m_enemy->GetActorLocation().ToString(), *m_player->GetActorLocation().ToString(), m_player->GetHP()));
        m_test->TestFalse(TEXT("alert releases movement"), controller->IsAlertReactionActive());
        m_enemy->TriggerAttack();
        m_enemy->Tick(1.0f / 30.0f);
        m_test->TestTrue(TEXT("attack starts after alert"), m_enemy->IsAttacking());
        m_test->TestTrue(TEXT("attack stops velocity"), m_enemy->GetVelocity().IsNearlyZero());
        const float health = m_player->GetHP();
        m_enemy->SetAttackCollisionEnabled(false);
        m_enemy->PerformAttackHit();
        m_test->TestEqual(TEXT("closed contact window cannot hurt player"), m_player->GetHP(), health);
        m_enemy->PrepareAttackContact(TEXT("RightHand"));
        m_enemy->ResetAttackHitForNewSwing();
        //姿勢の検証とは分け、正面の手足判定に接触する位置を明示して受付区間を試す。
        const FVector contact = m_enemy->GetActorLocation() + m_enemy->GetActorForwardVector() * 100.0f;
        m_enemy->FindComponentByClass<UBoxComponent>()->SetWorldLocation(contact);
        m_player->SetActorLocation(contact);
        m_enemy->SetAttackCollisionEnabled(true);
        m_enemy->TraceAttackContact();
        const float hitHealth = m_player->GetHP();
        m_test->TestTrue(TEXT("reachable player takes damage"), hitHealth < health);
        m_enemy->PerformAttackHit();
        m_test->TestEqual(TEXT("one hit per swing"), m_player->GetHP(), hitHealth);
        m_enemy->TakeDamage(100000.0f, FDamageEvent(), nullptr, m_player);
        m_test->TestTrue(TEXT("death interrupts attack"), m_enemy->IsDead() && !m_enemy->IsAttacking());
        m_test->TestEqual(TEXT("AI stops on death"), controller->GetCurrentState(), EEnemyAIState::Dead);
        m_enemy->PerformAttackHit();
        m_test->TestEqual(TEXT("death cannot deal damage"), m_player->GetHP(), hitHealth);
        GEngine->DestroyWorldContext(m_world);
        m_world->DestroyWorld(false);
        return true;
    }
};

//ボスの攻撃を最後まで進め、回復、段階移行、死亡の順に停止条件を検証する。
class FBossCombatFlowCommand : public IAutomationLatentCommand
{
    //各段階の失敗を記録するテスト。
    FAutomationTestBase* m_test;
    //タイマーとアニメーションを進める専用ワールド。
    UWorld* m_world;
    //通常攻撃、範囲攻撃、突進の実行対象。
    ABossChara* m_boss;
    //射程内の攻撃対象。
    APlayerChara* m_player;
    //現在検証している攻撃と、続く段階移行を区別する番号。
    int32 m_stage = 0;
    //終了しない攻撃を検出する待ち時間。
    int32 m_waitFrames = 0;
public:
    //一体ずつ独立したワールドで検証し、他のボスの知覚を混ぜない。
    FBossCombatFlowCommand(FAutomationTestBase* _test, UWorld* _world, ABossChara* _boss, APlayerChara* _player)
        : m_test(_test), m_world(_world), m_boss(_boss), m_player(_player) {}

    //各攻撃の終了を待ち、最後に死亡中断とワールドの後片付けを行う。
    virtual bool Update() override
    {
        m_world->Tick(LEVELTICK_All, 1.0f / 30.0f);
        m_boss->GetMesh()->TickAnimation(1.0f / 30.0f, false);
        m_boss->GetMesh()->RefreshBoneTransforms();
        if (m_waitFrames > 0)
        {
            if (!m_boss->CanPerformTacticalAction() && ++m_waitFrames < 360) { return false; }
            m_test->TestTrue(m_boss->GetName() + TEXT(" action releases combat lock"), m_boss->CanPerformTacticalAction());
            m_test->TestFalse(m_boss->GetName() + TEXT(" action releases common attack flag"), m_boss->IsAttacking());
            m_waitFrames = 0;
        }
        const EBossAttackPattern patterns[] = {EBossAttackPattern::LightCombo, EBossAttackPattern::PowerSlam, EBossAttackPattern::ChargeRush};
        if (m_stage < 3)
        {
            const EBossAttackPattern pattern = patterns[m_stage++];
            if (!m_boss->SupportsAttackPattern(pattern)) { return false; }
            const bool started = m_boss->RequestSpecificAttack(pattern, m_player->GetActorLocation());
            m_test->TestTrue(m_boss->GetName() + TEXT(" supported attack starts"), started);
            m_test->TestTrue(m_boss->GetName() + TEXT(" common attack flag set"), m_boss->IsAttacking());
            m_waitFrames = 1;
            return false;
        }
        if (m_stage++ == 3)
        {
            //途中で段階移行しても、以前の攻撃予約が再開しないことを確認する。
            m_boss->RequestSpecificAttack(EBossAttackPattern::LightCombo, m_player->GetActorLocation());
            m_boss->TakeDamage(m_boss->GetMaxHP() * 0.6f, FDamageEvent(), nullptr, m_player);
            m_test->TestTrue(m_boss->GetName() + TEXT(" phase transition starts"), m_boss->IsTransitioning());
            m_waitFrames = 1;
            return false;
        }
        m_boss->RequestSpecificAttack(EBossAttackPattern::LightCombo, m_player->GetActorLocation());
        m_boss->TakeDamage(100000.0f, FDamageEvent(), nullptr, m_player);
        m_test->TestTrue(m_boss->GetName() + TEXT(" dies during attack"), m_boss->IsDead());
        m_test->TestFalse(m_boss->GetName() + TEXT(" dead boss cannot act"), m_boss->CanPerformTacticalAction());
        m_test->TestFalse(m_boss->GetName() + TEXT(" death clears attack flag"), m_boss->IsAttacking());
        GEngine->DestroyWorldContext(m_world);
        m_world->DestroyWorld(false);
        return true;
    }
};

//攻撃の最中にIdleへ戻らず、元クリップの腰の向きと高さが全身へ反映されることを確認する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyAttackPoseTest, "ZombieAttack.Enemies.AttackPose",
                                EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyAttackPoseTest::RunTest(const FString& _parameters)
{
    UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& context = GEngine->CreateNewWorldContext(EWorldType::Game);
    context.SetCurrentWorld(world);
    for (const TCHAR* name : {TEXT("BP_Enemy"), TEXT("BP_MidBossChara"), TEXT("BP_FinalBossChara")})
    {
        const FString path = FString::Printf(TEXT("/Game/Blueprints/Enemy/Actors/%s.%s_C"), name, name);
        UClass* enemyClass = LoadClass<AEnemyChara>(nullptr, *path);
        if (!TestNotNull(path, enemyClass)) { continue; }
        FActorSpawnParameters spawn;
        spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEnemyChara* enemy = world->SpawnActor<AEnemyChara>(enemyClass, FVector::ZeroVector, FRotator::ZeroRotator, spawn);
        enemy->DispatchBeginPlay();
        TestTrue(FString(name) + TEXT(" hand socket"), enemy->PrepareAttackContact(TEXT("RightHand")));
        TestTrue(FString(name) + TEXT(" foot socket"), enemy->PrepareAttackContact(TEXT("RightFoot")));
        TestFalse(FString(name) + TEXT(" invalid socket rejected"), enemy->PrepareAttackContact(TEXT("MissingBone")));
        USkeletalMeshComponent* mesh = enemy->GetMesh();
        UAnimInstance* instance = mesh->GetAnimInstance();
        //BPに設定されている待機以外のモンタージュを、一つずつ最初から最後まで評価する。
        TSet<UAnimMontage*> montages;
        for (TFieldIterator<FObjectPropertyBase> property(enemyClass); property; ++property)
        {
            if (UAnimMontage* montage = Cast<UAnimMontage>(property->GetObjectPropertyValue_InContainer(enemy))) { montages.Add(montage); }
        }
        for (const TCHAR* field : {TEXT("m_comboAttackMontages"), TEXT("m_attackChoices")})
        {
            FArrayProperty* array = FindFProperty<FArrayProperty>(enemyClass, field);
            if (!array) { continue; }
            FScriptArrayHelper values(array, array->ContainerPtrToValuePtr<void>(enemy));
            FObjectPropertyBase* object = CastFieldChecked<FObjectPropertyBase>(array->Inner);
            for (int32 index = 0; index < values.Num(); ++index)
            {
                if (UAnimMontage* montage = Cast<UAnimMontage>(object->GetObjectPropertyValue(values.GetRawPtr(index)))) { montages.Add(montage); }
            }
        }
        for (UAnimMontage* montage : montages)
        {
            if (!montage || montage->SlotAnimTracks.IsEmpty() || montage->SlotAnimTracks[0].AnimTrack.AnimSegments.IsEmpty()) { continue; }
            UAnimSequence* clip = Cast<UAnimSequence>(montage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].GetAnimReference());
            if (!clip) { continue; }
            TestFalse(clip->GetName() + TEXT(" does not freeze hips"), clip->bForceRootLock);
            instance->Montage_Stop(0.0f);
            TestTrue(montage->GetName() + TEXT(" starts"), instance->Montage_Play(montage) > 0.0f);
            int32 compared = 0;
            for (int32 frame = 0; frame < FMath::CeilToInt(montage->GetPlayLength() * 30.0f) + 12; ++frame)
            {
                mesh->TickAnimation(1.0f / 30.0f, false);
                mesh->RefreshBoneTransforms();
                const float time = instance->Montage_GetPosition(montage);
                if (!instance->Montage_IsPlaying(montage) || instance->GetSlotMontageGlobalWeight(TEXT("DefaultSlot")) < 0.999f) { continue; }
                FTransform source;
                clip->GetBoneTransform(source, FSkeletonPoseBoneIndex(0), FAnimExtractContext(static_cast<double>(time)), false);
                const FTransform actual = mesh->GetComponentSpaceTransforms()[0];
                TestTrue(montage->GetName() + TEXT(" pelvis rotation preserved"), actual.GetRotation().Equals(source.GetRotation(), 0.01f));
                TestTrue(montage->GetName() + TEXT(" crouch height preserved"),
                    FMath::IsNearlyEqual(actual.GetLocation().Z, source.GetLocation().Z, 0.1f));
                ++compared;
            }
            TestTrue(montage->GetName() + TEXT(" reaches full action pose"), compared > 0);
            TestFalse(montage->GetName() + TEXT(" finishes without looping"), instance->Montage_IsPlaying(montage));
        }
        enemy->Destroy();
    }
    GEngine->DestroyWorldContext(world);
    world->DestroyWorld(false);
    return true;
}

//通常敵が発見、攻撃、硬直解除、死亡まで進み、同じ振りで二重にダメージを与えないことを確認する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyCombatFlowTest, "ZombieAttack.Enemies.CombatFlow",
                                EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyCombatFlowTest::RunTest(const FString& _parameters)
{
    UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& context = GEngine->CreateNewWorldContext(EWorldType::Game);
    context.SetCurrentWorld(world);
    world->SetGameInstance(NewObject<UGameInstance>(GEngine));
    FURL url;
    url.AddOption(TEXT("game=/Script/Engine.GameModeBase"));
    world->SetGameMode(url);
    APlayerController* playerController = world->SpawnActor<APlayerController>();
    APlayerChara* player = world->SpawnActor<APlayerChara>();
    playerController->Possess(player);
    UClass* enemyClass = LoadClass<AEnemyChara>(nullptr, TEXT("/Game/Blueprints/Enemy/Actors/BP_Enemy.BP_Enemy_C"));
    FActorSpawnParameters spawn;
    spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEnemyChara* enemy = world->SpawnActor<AEnemyChara>(enemyClass, FVector(-110, 0, 0), FRotator::ZeroRotator, spawn);
    enemy->DispatchBeginPlay();
    enemy->SpawnDefaultController();
    player->GetCharacterMovement()->SetMovementMode(MOVE_None);
    enemy->GetCharacterMovement()->SetMovementMode(MOVE_None);
    AEnemyAIController* controller = Cast<AEnemyAIController>(enemy->GetController());
    if (TestNotNull(TEXT("enemy controller"), controller))
    {
        controller->StartChase(player);
        TestTrue(TEXT("alert holds movement"), controller->IsAlertReactionActive());
        TestFalse(TEXT("alert does not start attack"), enemy->IsAttacking());
        ADD_LATENT_AUTOMATION_COMMAND(FEnemyCombatFlowCommand(this, world, enemy, player));
        return true;
    }
    GEngine->DestroyWorldContext(world);
    world->DestroyWorld(false);
    return true;
}

//中ボスと最終ボスの実際の攻撃資産を使い、攻撃終了と中断を通して確認する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossCombatFlowTest, "ZombieAttack.Enemies.BossCombatFlow",
                                EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBossCombatFlowTest::RunTest(const FString& _parameters)
{
    for (const TCHAR* name : {TEXT("BP_MidBossChara"), TEXT("BP_FinalBossChara")})
    {
        UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
        FWorldContext& context = GEngine->CreateNewWorldContext(EWorldType::Game);
        context.SetCurrentWorld(world);
        world->SetGameInstance(NewObject<UGameInstance>(GEngine));
        FURL url;
        url.AddOption(TEXT("game=/Script/Engine.GameModeBase"));
        world->SetGameMode(url);
        APlayerChara* player = world->SpawnActor<APlayerChara>();
        player->SetCanBeDamaged(false);
        player->GetCharacterMovement()->SetMovementMode(MOVE_None);
        const FString path = FString::Printf(TEXT("/Game/Blueprints/Enemy/Actors/%s.%s_C"), name, name);
        UClass* bossClass = LoadClass<ABossChara>(nullptr, *path);
        FActorSpawnParameters spawn;
        spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ABossChara* boss = world->SpawnActor<ABossChara>(bossClass, FVector(-140, 0, 0), FRotator::ZeroRotator, spawn);
        boss->DispatchBeginPlay();
        boss->SpawnDefaultController();
        boss->GetCharacterMovement()->SetMovementMode(MOVE_None);
        boss->SetCombatTarget(player);
        if (AEnemyAIController* controller = Cast<AEnemyAIController>(boss->GetController())) { controller->StopPatrol(); }
        ADD_LATENT_AUTOMATION_COMMAND(FBossCombatFlowCommand(this, world, boss, player));
    }
    return true;
}

//見失った直後の咆哮連打と、三体以上が同時に殴り始める状態を再現して検証する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyGroupTest, "ZombieAttack.Enemies.GroupSpacing",
                                EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyGroupTest::RunTest(const FString& _parameters)
{
    UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& context = GEngine->CreateNewWorldContext(EWorldType::Game);
    context.SetCurrentWorld(world);
    APlayerChara* player = world->SpawnActor<APlayerChara>();
    UClass* enemyClass = LoadClass<AEnemyChara>(nullptr, TEXT("/Game/Blueprints/Enemy/Actors/BP_Enemy.BP_Enemy_C"));
    TArray<AEnemyChara*> enemies;
    const FVector positions[] = {FVector(-120, 0, 0), FVector(120, 0, 0), FVector(0, 120, 0)};
    for (const FVector& position : positions)
    {
        FActorSpawnParameters spawn;
        spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEnemyChara* enemy = world->SpawnActor<AEnemyChara>(enemyClass, position, (-position).Rotation(), spawn);
        enemy->DispatchBeginPlay();
        enemy->SpawnDefaultController();
        enemies.Add(enemy);
    }
    //同じ瞬間の知覚通知でも一体だけが咆哮し、残りは追跡へ移る。
    CastChecked<AEnemyAIController>(enemies[0]->GetController())->StartChase(player);
    int32 roaring = 0;
    for (AEnemyChara* enemy : enemies)
    {
        auto* controller = CastChecked<AEnemyAIController>(enemy->GetController());
        controller->StartChase(player);
        if (controller->IsAlertReactionActive()) { ++roaring; }
    }
    TestEqual(TEXT("only one nearby enemy roars"), roaring, 1);
    for (AEnemyChara* enemy : enemies)
    {
        AEnemyAIController* controller = CastChecked<AEnemyAIController>(enemy->GetController());
        controller->StartChase(player);
        controller->StartSearch(player->GetActorLocation());
        TestFalse(TEXT("search cancels alert movement lock"), controller->IsAlertReactionActive());
        controller->StartChase(player);
        TestFalse(TEXT("recent reacquisition does not repeat scream"), controller->IsAlertReactionActive());
    }
    for (AEnemyChara* enemy : enemies)
    {
        enemy->TriggerAttack();
        enemy->Tick(1.0f / 30.0f);
    }
    TestTrue(TEXT("first approach can attack"), enemies[0]->IsAttacking());
    TestFalse(TEXT("opposite approach waits for stagger"), enemies[1]->IsAttacking());
    TestFalse(TEXT("third attacker waits outside"), enemies[2]->IsAttacking());
    //開始間隔を過ぎれば別方向の一体が加われるが、三体目は待機を続ける。
    player->GetCharacterMovement()->SetMovementMode(MOVE_None);
    for (AEnemyChara* enemy : enemies) { enemy->GetCharacterMovement()->SetMovementMode(MOVE_None); }
    //エンジンの一フレームの上限時間を超えない小刻みな更新で待機時間を進める。
    for (int32 step = 0; step < 3; ++step) { world->Tick(LEVELTICK_All, 0.25f); }
    enemies[1]->TriggerAttack();
    enemies[1]->Tick(1.0f / 30.0f);
    TestTrue(TEXT("opposite approach joins after stagger"), enemies[1]->IsAttacking());
    enemies[2]->TriggerAttack();
    enemies[2]->Tick(1.0f / 30.0f);
    TestFalse(TEXT("third attacker still waits"), enemies[2]->IsAttacking());
    GEngine->DestroyWorldContext(world);
    world->DestroyWorld(false);
    return true;
}

#endif
