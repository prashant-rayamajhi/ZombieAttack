#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "AssetCompilingManager.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "ZombieAttack/Enemy/EnemyChara.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimSequence.h"

//本編の配置カメラを順番に撮影し、露出を変えた後の森と進路を確認する。
class FForestLightingCapture : public IAutomationLatentCommand
{
public:
    FForestLightingCapture(FAutomationTestBase* _test, USceneCaptureComponent2D* _capture, const TArray<ACameraActor*>& _views)
        : m_test(_test), m_capture(_capture), m_views(_views) {}
    virtual bool Update() override
    {
        if (m_view >= m_views.Num())
        {
            for (AEnemyChara* enemy : m_previews) { enemy->Destroy(); }
            m_capture->GetOwner()->Destroy();
            return true;
        }
        ACameraActor* camera = m_views[m_view];
        //三種類の敵を同じ照明下へ仮配置し、木陰で顔や輪郭が消えていないか確認する。
        if (m_frame == 0)
        {
            if (m_previews.IsEmpty())
            {
                for (const TCHAR* name : {TEXT("BP_Enemy"), TEXT("BP_MidBossChara"), TEXT("BP_FinalBossChara")})
                {
                    const FString path = FString::Printf(TEXT("/Game/Blueprints/Enemy/Actors/%s.%s_C"), name, name);
                    UClass* cls = LoadClass<AEnemyChara>(nullptr, *path);
                    FActorSpawnParameters spawn;
                    spawn.ObjectFlags |= RF_Transient;
                    spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                    AEnemyChara* enemy = m_capture->GetWorld()->SpawnActor<AEnemyChara>(cls, spawn);
                    enemy->SetActorEnableCollision(false);
                    enemy->GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
                    enemy->GetMesh()->SetAnimation(enemy->GetIdleAnimation());
                    enemy->GetMesh()->Play(true);
                    m_previews.Add(enemy);
                }
            }
            for (int32 index = 0; index < m_previews.Num(); ++index)
            {
                FVector place = camera->GetActorLocation() + camera->GetActorForwardVector().GetSafeNormal2D() * 800.0f;
                place += camera->GetActorRightVector() * (index - 1) * 180.0f;
                FHitResult ground;
                const FVector start(place.X, place.Y, camera->GetActorLocation().Z + 100.0f);
                const FVector end(place.X, place.Y, camera->GetActorLocation().Z - 3000.0f);
                if (m_capture->GetWorld()->LineTraceSingleByChannel(ground, start, end, ECC_Visibility))
                {
                    place.Z = ground.ImpactPoint.Z + m_previews[index]->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
                }
                m_previews[index]->SetActorLocation(place);
                m_previews[index]->SetActorRotation(FRotator(0, camera->GetActorRotation().Yaw + 180.0f, 0));
            }
        }
        for (AEnemyChara* enemy : m_previews)
        {
            enemy->GetMesh()->TickAnimation(1.0f / 30.0f, false);
            enemy->GetMesh()->RefreshBoneTransforms();
        }
        m_capture->SetWorldTransform(camera->GetCameraComponent()->GetComponentTransform());
        m_capture->FOVAngle = camera->GetCameraComponent()->FieldOfView;
        m_capture->GetWorld()->SendAllEndOfFrameUpdates();
        m_capture->CaptureScene();
        if (++m_frame < 90) { return false; }
        FlushRenderingCommands();
        FImage image;
        m_test->TestTrue(TEXT("forest capture readable"), FImageUtils::GetRenderTargetImage(m_capture->TextureTarget, image));
        const FString folder = FPaths::ProjectSavedDir() / TEXT("Tests/ForestLighting");
        IFileManager::Get().MakeDirectory(*folder, true);
        m_test->TestTrue(TEXT("forest capture saved"),
            FImageUtils::SaveImageByExtension(*(folder / (camera->GetActorLabel() + TEXT(".png"))), image));
        ++m_view;
        m_frame = 0;
        return false;
    }
private:
    //画像の読み取りと保存が失敗していないかを報告するテスト。
    FAutomationTestBase* m_test;
    //本編のPostProcessVolumeをそのまま適用する撮影機材。
    USceneCaptureComponent2D* m_capture;
    //入口、戦闘区域、出口の確認地点。
    TArray<ACameraActor*> m_views;
    //露出と素材の読み込みを待つ経過フレーム。
    int32 m_frame = 0;
    //現在撮影している配置カメラの番号。
    int32 m_view = 0;
    //撮影後に破棄する確認用モデル。マップへは保存しない。
    TArray<AEnemyChara*> m_previews;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestLightingTest, "ZombieAttack.World.LightingViews",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FForestLightingTest::RunTest(const FString& _parameters)
{
    if (!FApp::CanEverRender()) { return true; }
    if (!TestTrue(TEXT("GameLevel1 opens"), FEditorFileUtils::LoadMap(TEXT("/Game/Map/GameLevel1"), false, true))) { return false; }
    UWorld* world = GEditor->GetEditorWorldContext().World();
    TArray<ACameraActor*> views;
    for (TActorIterator<ACameraActor> camera(world); camera; ++camera)
    {
        const FString name = camera->GetActorLabel();
        if (name == TEXT("CAM_PlayerIntro") || name == TEXT("CAM_Spawn2") || name == TEXT("CAM_Goal")) { views.Add(*camera); }
    }
    TestEqual(TEXT("three lighting viewpoints"), views.Num(), 3);
    FActorSpawnParameters spawn;
    spawn.ObjectFlags |= RF_Transient;
    AActor* rig = world->SpawnActor<AActor>(spawn);
    auto* capture = NewObject<USceneCaptureComponent2D>(rig);
    rig->SetRootComponent(capture);
    capture->RegisterComponent();
    capture->bCaptureEveryFrame = false;
    capture->bCaptureOnMovement = false;
    capture->bAlwaysPersistRenderingState = true;
    capture->CaptureSource = SCS_FinalColorLDR;
    auto* target = NewObject<UTextureRenderTarget2D>();
    target->InitCustomFormat(1280, 720, PF_B8G8R8A8, false);
    target->UpdateResourceImmediate();
    capture->TextureTarget = target;
    FAssetCompilingManager::Get().FinishAllCompilation();
    ADD_LATENT_AUTOMATION_COMMAND(FForestLightingCapture(this, capture, views));
    return true;
}
#endif
