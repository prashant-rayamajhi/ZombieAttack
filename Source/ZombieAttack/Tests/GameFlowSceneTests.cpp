#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ZombieAttack/UI/GameFlow/GameFlowScene.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "AssetCompilingManager.h"
#include "RenderingThread.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

//描画を実フレームで進め、Naniteとテクスチャの読み込み前の背景を完成画像と誤認しない。
class FMenuSceneCapture : public IAutomationLatentCommand
{
public:
    FMenuSceneCapture(FAutomationTestBase* _test, UWorld* _world, AGameFlowScene* _stage, USceneCaptureComponent2D* _capture)
        : m_test(_test), m_world(_world), m_stage(_stage), m_capture(_capture) {}
    virtual bool Update() override
    {
        m_world->Tick(LEVELTICK_All, 1.0f / 30.0f);
        //BeginPlayのない検証ワールドでも本番と同じ時間だけ人物の動作を進める。
        if (auto* player = m_stage->FindComponentByClass<USkeletalMeshComponent>())
        {
            player->TickAnimation(1.0f / 30.0f, false);
            player->RefreshBoneTransforms();
        }
        m_stage->Tick(1.0f / 30.0f);
        m_capture->SetWorldTransform(m_stage->GetCamera()->GetComponentTransform());
        m_world->SendAllEndOfFrameUpdates();
        m_capture->CaptureScene();
        if (++m_frame < 120) { return false; }
        FlushRenderingCommands();
        FImage image;
        m_test->TestTrue(TEXT("menu scene readback"), FImageUtils::GetRenderTargetImage(m_capture->TextureTarget, image));
        const FString folder = FPaths::ProjectSavedDir() / TEXT("Tests/MenuScenes");
        IFileManager::Get().MakeDirectory(*folder, true);
        m_test->TestTrue(TEXT("menu scene saved"),
            FImageUtils::SaveImageByExtension(*(folder / FString::Printf(TEXT("Scene%d.png"), m_scene)), image));
        if (++m_scene < 3)
        {
            m_frame = 0;
            m_stage->SetScene(m_scene);
            m_capture->PostProcessSettings = m_stage->GetCamera()->PostProcessSettings;
            return false;
        }
        GEngine->DestroyWorldContext(m_world);
        m_world->DestroyWorld(false);
        return true;
    }
private:
    FAutomationTestBase* m_test;
    UWorld* m_world;
    AGameFlowScene* m_stage;
    USceneCaptureComponent2D* m_capture;
    int32 m_frame = 0;
    int32 m_scene = 0;
};

//実際のメニュー用モデルと照明を描画し、三画面の背景を画像で確認する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameFlowSceneTest, "ZombieAttack.UI.MenuScene",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGameFlowSceneTest::RunTest(const FString& _parameters)
{
    if (!FApp::CanEverRender()) { return true; }
    UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
    AGameFlowScene* stage = world->SpawnActor<AGameFlowScene>();
    FAssetCompilingManager::Get().FinishAllCompilation();
    auto* capture = NewObject<USceneCaptureComponent2D>(stage);
    capture->RegisterComponent();
    capture->bCaptureEveryFrame = false;
    capture->bCaptureOnMovement = false;
    capture->CaptureSource = SCS_FinalColorLDR;
    auto* target = NewObject<UTextureRenderTarget2D>();
    target->InitCustomFormat(1280, 720, PF_B8G8R8A8, false);
    target->UpdateResourceImmediate();
    capture->TextureTarget = target;
    stage->SetScene(0);
    capture->SetWorldTransform(stage->GetCamera()->GetComponentTransform());
    capture->FOVAngle = stage->GetCamera()->FieldOfView;
    capture->PostProcessSettings = stage->GetCamera()->PostProcessSettings;
    capture->bAlwaysPersistRenderingState = true;
    ADD_LATENT_AUTOMATION_COMMAND(FMenuSceneCapture(this, world, stage, capture));
    return true;
}
#endif
