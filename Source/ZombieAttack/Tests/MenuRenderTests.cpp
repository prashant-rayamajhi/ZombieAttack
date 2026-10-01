#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "UObject/Package.h"
#include "ZombieAttack/UI/GameFlow/GameFlowScreenWidget.h"

//生成画像なしの三画面を実際のSlate描画で出力し、文字と背景の重なりを確認できるようにする。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMenuRenderTest, "ZombieAttack.UI.MenuRender",
                                EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMenuRenderTest::RunTest(const FString& _parameters)
{
    if (!FApp::CanEverRender()) { return true; }
    const FString folder = FPaths::ProjectSavedDir() / TEXT("Tests/MenuScreens");
    IFileManager::Get().MakeDirectory(*folder, true);
    for (const TCHAR* name : {TEXT("GameStart"), TEXT("GameClear"), TEXT("GameOver")})
    {
        UPackage* package = CreatePackage(*(FString(TEXT("/Temp/MenuPreview/")) + name));
        UWorld* world = UWorld::CreateWorld(EWorldType::Game, false, FName(name), package);
        FWorldContext& context = GEngine->CreateNewWorldContext(EWorldType::Game);
        context.SetCurrentWorld(world);
        APlayerController* controller = world->SpawnActor<APlayerController>();
        ULocalPlayer* player = NewObject<ULocalPlayer>(GEngine);
        controller->SetPlayer(player);
        const FString path = FString::Printf(TEXT("/Game/Blueprints/UI/GameFlow/WBP_%sScreen.WBP_%sScreen_C"), name, name);
        UClass* screenClass = LoadClass<UGameFlowScreenWidget>(nullptr, *path);
        if (TestNotNull(path, screenClass))
        {
            UGameFlowScreenWidget* screen = CreateWidget<UGameFlowScreenWidget>(controller, screenClass);
            if (TestNotNull(TEXT("Menu instance"), screen))
            {
                const TSharedRef<SWidget> widget = screen->TakeWidget();
                TestNotNull(TEXT("Menu root built for local player"), screen->WidgetTree->RootWidget.Get());
                TestNotNull(TEXT("Menu buttons built"), screen->WidgetTree->FindWidget(TEXT("PrimaryButton")));
                TestNotNull(TEXT("Procedural screen built"), screen->WidgetTree->FindWidget(TEXT("SceneObjective")));
                UTextBlock* title = Cast<UTextBlock>(screen->WidgetTree->FindWidget(TEXT("Title")));
                const FString expected = FString(name) == TEXT("GameClear") ? TEXT("MISSION COMPLETE") :
                    (FString(name) == TEXT("GameOver") ? TEXT("YOU DIED") : TEXT("ZOMBIE ATTACK"));
                if (TestNotNull(TEXT("Title label"), title)) { TestEqual(TEXT("Correct scene title"), title->GetText().ToString(), expected); }
                FWidgetRenderer renderer(false);
                UTextureRenderTarget2D* target = FWidgetRenderer::CreateTargetFor(FVector2D(1920, 1080), TF_Bilinear, false);
                const FGeometry geometry = FGeometry::MakeRoot(FVector2D(1920, 1080), FSlateLayoutTransform());
                FImage firstFrame;
                for (int32 frame = 0; frame < 60; ++frame)
                {
                    //ウィンドウを開かない描画でも、通常のSlateと同じ更新経路へ時間を渡す。
                    widget->Tick(geometry, frame / 30.0, 1.0f / 30.0f);
                    screen->WidgetTree->ForEachWidget([&](UWidget* _child)
                    {
                        if (UUserWidget* child = Cast<UUserWidget>(_child))
                        {
                            child->TakeWidget()->Tick(geometry, frame / 30.0, 1.0f / 30.0f);
                        }
                    });
                    renderer.DrawWidget(target, widget, FVector2D(1920, 1080), 1.0f / 30.0f);
                    FlushRenderingCommands();
                    if (frame == 0) { FImageUtils::GetRenderTargetImage(target, firstFrame); }
                }
                FImage output;
                const bool captured = FImageUtils::GetRenderTargetImage(target, output);
                TestTrue(FString(name) + TEXT(" rendered"), captured);
                if (captured)
                {
                    TestTrue(FString(name) + TEXT(" animation changes the image"), firstFrame.RawData != output.RawData);
                    TestTrue(FString(name) + TEXT(" saved"), FImageUtils::SaveImageByExtension(*(folder / (FString(name) + TEXT(".png"))), output));
                }
                screen->ReleaseSlateResources(true);
            }
        }
        GEngine->DestroyWorldContext(world);
        world->DestroyWorld(false);
    }
    return true;
}

#endif
