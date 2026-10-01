#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "Engine/GameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ZombieAttack/Enemy/EnemyChara.h"
#include "ZombieAttack/Enemy/BossChara/BossChara.h"
#include "ZombieAttack/AIController/BossUtilityAIComponent.h"
#include "ZombieAttack/Animation/Enemy/EnemyAnimInstance.h"
#include "ZombieAttack/UI/PlayerUI/WeaponSlotEntryWidget.h"
#include "UObject/UnrealType.h"

//敵種別の取り違えと、全身スロットの設定漏れをアセット読込時に検出する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyAnimationAssetsTest, "ZombieAttack.Enemies.AnimationAssets",
                                EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyAnimationAssetsTest::RunTest(const FString& _parameters)
{
    const TCHAR* names[] = {TEXT("BP_Enemy"), TEXT("BP_MidBossChara"), TEXT("BP_FinalBossChara")};
    for (const TCHAR* name : names)
    {
        const FString path = FString::Printf(TEXT("/Game/Blueprints/Enemy/Actors/%s.%s_C"), name, name);
        UClass* enemyClass = LoadClass<AEnemyChara>(nullptr, *path);
        if (!TestNotNull(path, enemyClass)) { continue; }
        const AEnemyChara* enemy = enemyClass->GetDefaultObject<AEnemyChara>();
        const USkeletalMesh* mesh = enemy->GetMesh()->GetSkeletalMeshAsset();
        if (!TestNotNull(TEXT("Enemy mesh"), mesh)) { continue; }
        for (const UAnimSequence* sequence : {enemy->GetIdleAnimation(), enemy->GetWalkAnimation(), enemy->GetRunAnimation()})
        {
            if (!TestNotNull(TEXT("Locomotion clip"), sequence)) { continue; }
            TestTrue(sequence->GetPathName() + TEXT(" skeleton"), sequence->GetSkeleton() == mesh->GetSkeleton());
            TestTrue(sequence->GetPathName() + TEXT(" duration"), sequence->GetPlayLength() > 0.0f);
            TestFalse(sequence->GetPathName() + TEXT(" pelvis is not frozen"), sequence->bForceRootLock);
        }
        for (TFieldIterator<FObjectPropertyBase> property(enemyClass); property; ++property)
        {
            const UAnimMontage* montage = Cast<UAnimMontage>(property->GetObjectPropertyValue_InContainer(enemy));
            if (!montage) { continue; }
            TestTrue(montage->GetPathName() + TEXT(" skeleton"), montage->GetSkeleton() == mesh->GetSkeleton());
            TestTrue(montage->GetPathName() + TEXT(" slot exists"), montage->SlotAnimTracks.Num() > 0);
            for (const FSlotAnimationTrack& track : montage->SlotAnimTracks)
            {
                TestEqual(montage->GetPathName() + TEXT(" slot"), track.SlotName, FName(TEXT("DefaultSlot")));
                for (const FAnimSegment& segment : track.AnimTrack.AnimSegments)
                {
                    const UAnimSequenceBase* clip = segment.GetAnimReference();
                    TestTrue(montage->GetPathName() + TEXT(" segment skeleton"), clip && clip->GetSkeleton() == mesh->GetSkeleton());
                }
            }
        }
    }
    return true;
}

//画面追加前のConfigureでも、Text Blockが残らず武器画像が設定されることを確認する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponIconBindingTest, "ZombieAttack.UI.WeaponIcons",
                                EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponIconBindingTest::RunTest(const FString& _parameters)
{
    UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& context = GEngine->CreateNewWorldContext(EWorldType::Game);
    context.SetCurrentWorld(world);
    UClass* widgetClass = LoadClass<UWeaponSlotEntryWidget>(nullptr, TEXT("/Game/Blueprints/UI/WBP_WeaponSlotEntry.WBP_WeaponSlotEntry_C"));
    if (TestNotNull(TEXT("Weapon entry Blueprint"), widgetClass))
    {
        UWeaponSlotEntryWidget* entry = CreateWidget<UWeaponSlotEntryWidget>(world, widgetClass);
        if (TestNotNull(TEXT("Weapon entry instance"), entry))
        {
            for (const TCHAR* name : {TEXT("T_Pistol_Icon"), TEXT("T_AR_Icon"), TEXT("T_Knife_Icon")})
            {
                const FString path = FString::Printf(TEXT("/Game/UI/WeaponIcons/%s.%s"), name, name);
                UTexture2D* texture = LoadObject<UTexture2D>(nullptr, *path);
                TestNotNull(path, texture);
                entry->ConfigureEntry(EWeaponSlot::Pistol, FText::FromString(TEXT("PISTOL")), texture, true);
                UImage* image = Cast<UImage>(entry->WidgetTree->FindWidget(TEXT("WeaponIcon")));
                UTextBlock* label = Cast<UTextBlock>(entry->WidgetTree->FindWidget(TEXT("WeaponNameText")));
                if (TestNotNull(TEXT("WeaponIcon binding"), image))
                {
                    TestTrue(TEXT("Icon brush uses supplied texture"), image->GetBrush().GetResourceObject() == texture);
                }
                if (TestNotNull(TEXT("WeaponNameText binding"), label))
                {
                    TestEqual(TEXT("Default Text Block replaced"), label->GetText().ToString(), FString(TEXT("PISTOL")));
                }
            }
        }
    }
    GEngine->DestroyWorldContext(world);
    world->DestroyWorld(false);
    return true;
}

//速度入力で姿勢が変わり、死亡後は待機へ戻らないことを全敵種で確認する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyPoseTest, "ZombieAttack.Enemies.RuntimePose",
                                EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyPoseTest::RunTest(const FString& _parameters)
{
    UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& context = GEngine->CreateNewWorldContext(EWorldType::Game);
    context.SetCurrentWorld(world);
    //Pawnのダメージ受付には権限を持つGameModeが必要なため、検証用の最小構成を用意する。
    UGameInstance* game = NewObject<UGameInstance>(GEngine);
    world->SetGameInstance(game);
    FURL url;
    url.AddOption(TEXT("game=/Script/Engine.GameModeBase"));
    world->SetGameMode(url);
    const TCHAR* names[] = {TEXT("BP_Enemy"), TEXT("BP_MidBossChara"), TEXT("BP_FinalBossChara")};
    for (const TCHAR* name : names)
    {
        const FString path = FString::Printf(TEXT("/Game/Blueprints/Enemy/Actors/%s.%s_C"), name, name);
        UClass* enemyClass = LoadClass<AEnemyChara>(nullptr, *path);
        if (!TestNotNull(TEXT("Enemy class"), enemyClass)) { continue; }
        FActorSpawnParameters spawn;
        spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEnemyChara* enemy = world->SpawnActor<AEnemyChara>(enemyClass, FVector::ZeroVector, FRotator::ZeroRotator, spawn);
        if (!TestNotNull(TEXT("Enemy instance"), enemy)) { continue; }
        enemy->DispatchBeginPlay();
        USkeletalMeshComponent* mesh = enemy->GetMesh();
        TestTrue(FString(name) + TEXT(" native anim instance"), mesh->GetAnimInstance()->IsA<UEnemyAnimInstance>());
        mesh->InitAnim(true);
        for (int32 frame = 0; frame < 20; ++frame)
        {
            mesh->TickAnimation(1.0f / 30.0f, false);
            mesh->RefreshBoneTransforms();
        }
        const TArray<FTransform> idle = mesh->GetComponentSpaceTransforms();
        enemy->GetCharacterMovement()->Velocity = FVector(390.0f, 0.0f, 0.0f);
        mesh->InitAnim(true);
        for (int32 frame = 0; frame < 20; ++frame)
        {
            mesh->TickAnimation(1.0f / 30.0f, false);
            mesh->RefreshBoneTransforms();
        }
        const TArray<FTransform>& moving = mesh->GetComponentSpaceTransforms();
        int32 changedBones = 0;
        for (int32 bone = 0; bone < FMath::Min(idle.Num(), moving.Num()); ++bone)
        {
            if (!idle[bone].Equals(moving[bone], 0.001f)) { ++changedBones; }
        }
        TestTrue(FString(name) + TEXT(" moving pose differs from idle"), changedBones > 10);
        TestTrue(FString(name) + TEXT(" alert plays"), enemy->PlayAlertAnimation() > 0.0f);
        for (int32 frame = 0; frame < 10; ++frame)
        {
            mesh->TickAnimation(1.0f / 30.0f, false);
            mesh->RefreshBoneTransforms();
        }
        TestTrue(FString(name) + TEXT(" montage reaches full body slot"),
                 mesh->GetAnimInstance()->GetSlotMontageGlobalWeight(TEXT("DefaultSlot")) > 0.5f);
        enemy->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
        TestTrue(FString(name) + TEXT(" death state"), enemy->IsDead());
        TestEqual(FString(name) + TEXT(" isolated death clip"), mesh->GetAnimationMode(), EAnimationMode::AnimationSingleNode);
        TestFalse(FString(name) + TEXT(" cannot attack after death"), enemy->IsAttacking());
        enemy->Destroy();
    }
    GEngine->DestroyWorldContext(world);
    world->DestroyWorld(false);
    return true;
}

//射程外や遮蔽物の向こうへ攻撃を選ばないことを、乱数を含めて繰り返し確認する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossDecisionTest, "ZombieAttack.Enemies.BossDecisions",
                                EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBossDecisionTest::RunTest(const FString& _parameters)
{
    for (const TCHAR* name : {TEXT("BP_MidBossChara"), TEXT("BP_FinalBossChara")})
    {
        const FString path = FString::Printf(TEXT("/Game/Blueprints/Enemy/Actors/%s.%s_C"), name, name);
        UClass* bossClass = LoadClass<ABossChara>(nullptr, *path);
        if (!TestNotNull(TEXT("Boss class"), bossClass)) { continue; }
        ABossChara* boss = bossClass->GetDefaultObject<ABossChara>();
        UBossUtilityAIComponent* decision = NewObject<UBossUtilityAIComponent>();
        decision->ConfigureForArchetype(boss->GetBossAIArchetype());
        FBossDecisionContext situation;
        situation.m_bossHealthRatio = 1.0f;
        situation.m_playerHealthRatio = 0.3f;
        situation.m_bPlayerReloading = true;
        for (int32 trial = 0; trial < 100; ++trial)
        {
            situation.m_distanceToPlayer = 5000.0f;
            situation.m_bHasLineOfSight = true;
            TestEqual(FString(name) + TEXT(" close distance before attacking"), decision->ChooseAction(situation, boss),
                      EBossTacticalAction::Approach);
            situation.m_distanceToPlayer = 80.0f;
            situation.m_bHasLineOfSight = false;
            const EBossTacticalAction action = decision->ChooseAction(situation, boss);
            const bool attack = action == EBossTacticalAction::LightCombo || action == EBossTacticalAction::PowerSlam ||
                                action == EBossTacticalAction::ChargeRush || action == EBossTacticalAction::BackStep;
            TestFalse(FString(name) + TEXT(" no attack through cover"), attack);
        }
    }
    return true;
}

#endif
