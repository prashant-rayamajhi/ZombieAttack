#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RenderingThread.h"
#include "AssetCompilingManager.h"
#include "UObject/UnrealType.h"
#include "ZombieAttack/Enemy/EnemyChara.h"
#include "ZombieAttack/Animation/Enemy/EnemyAnimInstance.h"

//各敵の実際のクリップを同じ照明で撮影し、腰の固定による姿勢の変化を比較する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyAnimationVisualTest, "ZombieAttack.Enemies.VisualClips",
                                EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyAnimationVisualTest::RunTest(const FString& _parameters)
{
    if (!FApp::CanEverRender()) { return true; }
    const FString folder = FPaths::ProjectSavedDir() / TEXT("Tests/EnemyClips");
    IFileManager::Get().MakeDirectory(*folder, true);
    FString report;
    UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& context = GEngine->CreateNewWorldContext(EWorldType::Game);
    context.SetCurrentWorld(world);
    //背景の暗さで関節が隠れないよう、前後から一定の光を当てる。
    for (int32 side = 0; side < 2; ++side)
    {
        ADirectionalLight* light = world->SpawnActor<ADirectionalLight>();
        light->SetActorRotation(FRotator(-35.0f, side == 0 ? -35.0f : 150.0f, 0.0f));
        light->GetLightComponent()->SetIntensity(side == 0 ? 5.0f : 2.0f);
    }
    AActor* camera = world->SpawnActor<AActor>();
    USceneCaptureComponent2D* capture = NewObject<USceneCaptureComponent2D>(camera);
    camera->SetRootComponent(capture);
    capture->RegisterComponent();
    capture->bCaptureEveryFrame = false;
    capture->bCaptureOnMovement = false;
    capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    capture->FOVAngle = 45.0f;
    capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
    capture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
    capture->PostProcessSettings.bOverride_AutoExposureBias = true;
    capture->PostProcessSettings.AutoExposureBias = 1.0f;
    capture->PostProcessSettings.bOverride_MotionBlurAmount = true;
    capture->PostProcessSettings.MotionBlurAmount = 0.0f;
    UTextureRenderTarget2D* target = NewObject<UTextureRenderTarget2D>();
    target->InitCustomFormat(384, 384, PF_B8G8R8A8, false);
    target->ClearColor = FLinearColor(0.04f, 0.04f, 0.04f);
    target->UpdateResourceImmediate();
    capture->TextureTarget = target;
    for (const TCHAR* name : {TEXT("BP_Enemy"), TEXT("BP_MidBossChara"), TEXT("BP_FinalBossChara")})
    {
        const FString path = FString::Printf(TEXT("/Game/Blueprints/Enemy/Actors/%s.%s_C"), name, name);
        UClass* enemyClass = LoadClass<AEnemyChara>(nullptr, *path);
        if (!TestNotNull(path, enemyClass)) { continue; }
        FActorSpawnParameters spawn;
        spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEnemyChara* enemy = world->SpawnActor<AEnemyChara>(enemyClass, FVector(0, 0, 95), FRotator::ZeroRotator, spawn);
        USkeletalMeshComponent* mesh = enemy->GetMesh();
        FAssetCompilingManager::Get().FinishAllCompilation();
        mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        mesh->bEnableUpdateRateOptimizations = false;
        report += FString::Printf(TEXT("\n%s root=%s scale=%s\n"), name, *mesh->GetBoneName(0).ToString(), *mesh->GetComponentScale().ToString());
        //移動クリップに加え、BPに指定された全攻撃と死亡モンタージュを列挙する。
        TSet<UAnimSequence*> clips;
        TSet<UAnimMontage*> montages;
        for (TFieldIterator<FObjectPropertyBase> property(enemyClass); property; ++property)
        {
            UObject* asset = property->GetObjectPropertyValue_InContainer(enemy);
            if (UAnimSequence* clip = Cast<UAnimSequence>(asset)) { clips.Add(clip); }
            if (UAnimMontage* montage = Cast<UAnimMontage>(asset)) { montages.Add(montage); }
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
            report += FString::Printf(TEXT("Montage %s length=%.3f blendin=%.3f blendout=%.3f\n"), *montage->GetName(),
                montage->GetPlayLength(), montage->BlendIn.GetBlendTime(), montage->BlendOut.GetBlendTime());
            for (const FAnimNotifyEvent& event : montage->Notifies)
            {
                report += FString::Printf(TEXT("  Notify %s at %.3f duration %.3f\n"), *event.GetNotifyEventName().ToString(),
                    event.GetTriggerTime(), event.GetDuration());
            }
            for (const FSlotAnimationTrack& track : montage->SlotAnimTracks)
            {
                for (const FAnimSegment& segment : track.AnimTrack.AnimSegments)
                {
                    report += FString::Printf(TEXT("  %s start=%.3f end=%.3f rate=%.3f loops=%d\n"),
                        *GetNameSafe(segment.GetAnimReference()), segment.AnimStartTime, segment.AnimEndTime,
                        segment.AnimPlayRate, segment.LoopingCount);
                    if (UAnimSequence* clip = Cast<UAnimSequence>(segment.GetAnimReference())) { clips.Add(clip); }
                }
            }
        }
        const FVector focus(0, 0, 100);
        const FVector view(330, 260, 160);
        capture->SetWorldLocationAndRotation(view, (focus - view).Rotation());
        for (UAnimSequence* clip : clips)
        {
            const bool oldLock = clip->bForceRootLock;
            const bool oldMotion = clip->bEnableRootMotion;
            clip->bEnableRootMotion = false;
            for (int32 lock = 0; lock < 3; ++lock)
            {
                clip->bForceRootLock = lock == 1;
                UAnimMontage* preview = nullptr;
                const bool death = clip->GetName().Contains(TEXT("Death")) || clip->GetName().Contains(TEXT("Dying"));
                if (lock == 2 && !death)
                {
                    //修正後の列は実ゲームと同じ全身スロットを通して撮影する。
                    mesh->SetAnimInstanceClass(UEnemyAnimInstance::StaticClass());
                    preview = mesh->GetAnimInstance()->PlaySlotAnimationAsDynamicMontage(clip, TEXT("DefaultSlot"), 0.01f, 0.01f);
                }
                else
                {
                    mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
                    mesh->PlayAnimation(clip, false);
                }
                const bool kicking = clip->GetName() == TEXT("Zombie_Kicking");
                for (int32 frame = 0; frame < (kicking ? 20 : 4); ++frame)
                {
                    //蹴りは足が伸びる短い区間を細かく撮り、接触通知の位置を確認する。
                    const float fraction = kicking ? 0.025f + frame * 0.05f : 0.1f + frame * 0.25f;
                    const float time = clip->GetPlayLength() * fraction;
                    if (preview) { mesh->GetAnimInstance()->Montage_SetPosition(preview, time); }
                    else { mesh->SetPosition(time, false); }
                    mesh->TickAnimation(preview ? 0.02f : 0.0f, false);
                    mesh->RefreshBoneTransforms();
                    mesh->UpdateComponentToWorld();
                    //同じ描画フレーム内でも、変更後の骨配列でスキニングを作り直す。
                    mesh->MarkRenderStateDirty();
                    const FTransform root = mesh->GetComponentSpaceTransforms()[0];
                    report += FString::Printf(TEXT("%s lock%d t=%.3f root=%s rot=%s\n"), *clip->GetName(), lock, time,
                        *root.GetLocation().ToString(), *root.Rotator().ToString());
                    if (kicking && lock == 2)
                    {
                        report += FString::Printf(TEXT("Kick contact t=%.3f LeftFoot=%s RightFoot=%s\n"), time,
                            *mesh->GetSocketLocation(TEXT("LeftFoot")).ToString(), *mesh->GetSocketLocation(TEXT("RightFoot")).ToString());
                    }
                    world->SendAllEndOfFrameUpdates();
                    capture->CaptureScene();
                    FlushRenderingCommands();
                    FImage image;
                    if (FImageUtils::GetRenderTargetImage(target, image))
                    {
                        const FString file = FString::Printf(TEXT("%s_%s_lock%d_%d.png"), name, *clip->GetName(), lock, frame);
                        TestTrue(file, FImageUtils::SaveImageByExtension(*(folder / file), image));
                    }
                }
            }
            clip->bForceRootLock = oldLock;
            clip->bEnableRootMotion = oldMotion;
        }
        enemy->Destroy();
    }
    FFileHelper::SaveStringToFile(report, *(folder / TEXT("Audit.txt")), FFileHelper::EEncodingOptions::ForceUTF8);
    GEngine->DestroyWorldContext(world);
    world->DestroyWorld(false);
    return true;
}

#endif
