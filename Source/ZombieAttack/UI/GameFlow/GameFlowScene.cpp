#include "GameFlowScene.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AGameFlowScene::AGameFlowScene()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Stage")));
    //本編の樹木を奥へ重ね、背景の絵柄を統一する。
    static ConstructorHelpers::FObjectFinder<UStaticMesh> tree(TEXT("/Game/Rain_Forest/Meshes/Vegetations/SM_AmurCork03"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> rock(TEXT("/Game/Rain_Forest/Meshes/Landscape/SM_Rock05"));
    for (int32 index = 0; index < 16; ++index)
    {
        auto* trunk = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Forest%d"), index));
        trunk->SetupAttachment(RootComponent);
        trunk->SetStaticMesh(tree.Object);
        trunk->SetRelativeLocation(FVector(350 + (index / 4) * 340, (index % 4) * 420 - 650, -30));
        trunk->SetRelativeRotation(FRotator(0, index * 71, 0));
        trunk->SetRelativeScale3D(FVector(2.2f + (index % 3) * 0.4f));
        trunk->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    //地表を一枚で閉じ、岩の隙間から空が見えるのを防ぐ。
    static ConstructorHelpers::FObjectFinder<UStaticMesh> cube(TEXT("/Engine/BasicShapes/Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> stone(TEXT("/Game/Rain_Forest/Materials/Landscape/MI_Stone01"));
    auto* floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
    floor->SetupAttachment(RootComponent);
    floor->SetStaticMesh(cube.Object);
    floor->SetMaterial(0, stone.Object);
    floor->SetRelativeLocation(FVector(0, 0, -18));
    floor->SetRelativeScale3D(FVector(600, 600, 0.2f));
    floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    //下草をまとめて描画し、平らな地面を隠しながら描画負荷を増やしすぎない。
    static ConstructorHelpers::FObjectFinder<UStaticMesh> grassMesh(TEXT("/Game/Rain_Forest/Meshes/Vegetations/SM_Grass12_1"));
    auto* grass = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Undergrowth"));
    grass->SetupAttachment(RootComponent);
    grass->SetStaticMesh(grassMesh.Object);
    grass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FRandomStream plants(4107);
    for (int32 index = 0; index < 500; ++index)
    {
        const FVector position(plants.FRandRange(-250, 1800), plants.FRandRange(-1200, 1200), -8);
        //プレイヤーの足元は空け、浮いているように見えない接地面を残す。
        if (FVector::DistSquared2D(position, FVector(40, 60, -8)) < 10000) { continue; }
        grass->AddInstance(FTransform(FRotator(0, plants.FRandRange(0, 360), 0), position, FVector(plants.FRandRange(2.5f, 4.0f))));
    }
    //岩の凹凸を地表に使い、単色の平面が見えるのを避ける。
    for (int32 index = 0; index < 6; ++index)
    {
        auto* ground = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Ground%d"), index));
        ground->SetupAttachment(RootComponent);
        ground->SetStaticMesh(rock.Object);
        ground->SetRelativeLocation(FVector((index / 2) * 500, (index % 2) * 600 - 300, -100));
        ground->SetRelativeScale3D(FVector(2, 2, 0.5f));
        ground->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    //奥に岩壁を重ね、地面の端が水平線のように見える人工的な背景を隠す。
    for (int32 index = 0; index < 8; ++index)
    {
        auto* ridge = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Ridge%d"), index));
        ridge->SetupAttachment(RootComponent);
        ridge->SetStaticMesh(rock.Object);
        ridge->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (rock.Object)
        {
            const FBoxSphereBounds bounds = rock.Object->GetBounds();
            const FVector scale = FVector(500, 480, 380 + (index % 3) * 70) / bounds.BoxExtent;
            const FVector bottom(bounds.Origin.X, bounds.Origin.Y, bounds.Origin.Z - bounds.BoxExtent.Z);
            ridge->SetRelativeScale3D(scale);
            ridge->SetRelativeLocation(FVector(1900, index * 650 - 2200, -25) - bottom * scale);
        }
    }
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> mesh(TEXT("/Game/Assets/Player/Mesh/Ch35_nonPBR"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> idle(TEXT("/Game/Assets/Player/Animation/AnimSequence/Warrior_Idle"));
    m_player = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Player"));
    m_player->SetupAttachment(RootComponent);
    m_player->SetSkeletalMesh(mesh.Object);
    m_player->SetRelativeLocation(FVector(40, 60, 0));
    m_player->SetRelativeRotation(FRotator(0, 70, 0));
    m_player->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    m_player->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    m_idle = idle.Object;
    static ConstructorHelpers::FObjectFinder<UAnimSequence> death(TEXT("/Game/Assets/Player/Animation/AnimSequence/Falling_Back_Death"));
    m_death = death.Object;
    m_player->AnimationData.AnimToPlay = idle.Object;
    m_player->AnimationData.bSavedLooping = true;
    m_player->AnimationData.bSavedPlaying = true;
    m_player->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    //本編と同じ街灯を置き、暖色の光がどこから来ているか分かる背景にする。
    static ConstructorHelpers::FObjectFinder<UStaticMesh> streetLamp(TEXT("/Game/Fab/Street_Lamp/street_lamp"));
    auto* lampPost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LampPost"));
    lampPost->SetupAttachment(RootComponent);
    lampPost->SetStaticMesh(streetLamp.Object);
    //素材の原点が支柱の底からずれているため、実際の境界から接地位置を合わせる。
    if (streetLamp.Object)
    {
        const FBoxSphereBounds bounds = streetLamp.Object->GetBounds();
        const FVector base(bounds.Origin.X, bounds.Origin.Y, bounds.Origin.Z - bounds.BoxExtent.Z);
        lampPost->SetRelativeLocation(FVector(180, 240, -8) - base);
    }
    lampPost->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    m_lamp = CreateDefaultSubobject<UPointLightComponent>(TEXT("Lantern"));
    m_lamp->SetupAttachment(RootComponent);
    m_lamp->SetRelativeLocation(FVector(-180, 100, 220));
    m_lamp->SetIntensity(5000);
    m_lamp->SetAttenuationRadius(1400);
    m_lamp->SetLightColor(FLinearColor(1, 0.55f, 0.26f));
    //逆光で木とプレイヤーの輪郭を拾い、暗部が黒一色になるのを防ぐ。
    auto* moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
    moon->SetupAttachment(RootComponent);
    moon->SetRelativeRotation(FRotator(-35, 135, 0));
    moon->SetIntensity(0.65f);
    moon->SetLightColor(FLinearColor(0.35f, 0.50f, 0.68f));
    //顔と装備に薄い反射光を入れ、背景より人物が暗く沈むのを防ぐ。
    auto* fill = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("PlayerFill"));
    fill->SetupAttachment(RootComponent);
    fill->SetRelativeRotation(FRotator(-20, 25, 0));
    fill->SetIntensity(0.3f);
    fill->SetCastShadows(false);
    fill->SetLightColor(FLinearColor(0.65f, 0.73f, 0.85f));
    //画面右の人物だけを正面上方から照らし、森の暗さを残したまま顔と装備を読めるようにする。
    auto* portraitLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("PlayerSpotlight"));
    portraitLight->SetupAttachment(RootComponent);
    const FVector lightPosition(-250, -100, 330);
    portraitLight->SetRelativeLocation(lightPosition);
    portraitLight->SetRelativeRotation((FVector(40, 60, 70) - lightPosition).Rotation());
    portraitLight->SetIntensity(18000.0f);
    portraitLight->SetAttenuationRadius(850.0f);
    portraitLight->SetInnerConeAngle(24.0f);
    portraitLight->SetOuterConeAngle(38.0f);
    portraitLight->SetLightColor(FLinearColor(0.88f, 0.93f, 1.0f));
    portraitLight->SetCastShadows(false);
    auto* fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("ForestFog"));
    fog->SetupAttachment(RootComponent);
    fog->SetFogDensity(0.025f);
    fog->SetFogInscatteringColor(FLinearColor(0.003f, 0.008f, 0.012f));
    m_camera = CreateDefaultSubobject<UCameraComponent>(TEXT("MenuCamera"));
    m_camera->SetupAttachment(RootComponent);
    m_camera->SetRelativeLocation(FVector(-560, -220, 160));
    m_camera->SetRelativeRotation((FVector(120, -100, 110) - m_camera->GetRelativeLocation()).Rotation());
    m_camera->FieldOfView = 52;
    m_camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
    m_camera->PostProcessSettings.AutoExposureMethod = AEM_Manual;
    m_camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    m_camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
    m_camera->PostProcessSettings.bOverride_AutoExposureBias = true;
    m_camera->PostProcessSettings.AutoExposureBias = 1;
    m_camera->PostProcessSettings.bOverride_VignetteIntensity = true;
    m_camera->PostProcessSettings.VignetteIntensity = 0.35f;
}

void AGameFlowScene::SetScene(int32 _scene)
{
    m_scene = _scene;
    //敗北時は倒れる動作を一度だけ再生し、完了後の姿勢を保つ。
    m_player->PlayAnimation(_scene == 2 ? m_death.Get() : m_idle.Get(), _scene != 2);
    m_player->SetVisibility(true);
    m_lamp->SetLightColor(_scene == 2 ? FLinearColor(1, 0.12f, 0.06f) : FLinearColor(1, 0.55f, 0.26f));
    m_camera->PostProcessSettings.AutoExposureBias = _scene == 1 ? 0.8f : -0.3f;
}

void AGameFlowScene::Tick(float _deltaTime)
{
    Super::Tick(_deltaTime);
    m_time += _deltaTime;
    m_camera->SetRelativeLocation(FVector(-560, -220 + FMath::Sin(m_time * 0.12f) * 8, 160));
    m_lamp->SetIntensity(5000 * (m_scene == 2 ? 0.88f + 0.12f * FMath::Sin(m_time * 2.1f) : 1));
}
