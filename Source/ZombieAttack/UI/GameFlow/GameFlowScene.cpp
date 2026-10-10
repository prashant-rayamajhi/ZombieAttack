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
        //木を通路の左右へずらして並べ、中央から奥へ視線が抜ける森にする。
        const float side = index % 2 == 0 ? -1.0f : 1.0f;
        trunk->SetRelativeLocation(FVector(480 + (index / 2) * 185, side * (400 + (index % 3) * 130), -30));
        trunk->SetRelativeRotation(FRotator(0, index * 71, 0));
        trunk->SetRelativeScale3D(FVector(1.6f + (index % 3) * 0.3f));
        trunk->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    //地表を敷き詰め、岩の隙間から空が見えるのを防ぐ。
    static ConstructorHelpers::FObjectFinder<UStaticMesh> cube(TEXT("/Engine/BasicShapes/Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> stone(TEXT("/Game/Rain_Forest/Materials/Landscape/MI_Ground02"));
    auto* floor = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ForestFloor"));
    floor->SetupAttachment(RootComponent);
    floor->SetStaticMesh(cube.Object);
    floor->SetMaterial(0, stone.Object);
    floor->SetRelativeLocation(FVector(0, 0, -18));
    //巨大な一枚のUVで落ち葉が引き伸ばされないよう、4m単位で土の面を敷く。
    for (int32 row = -3; row <= 10; ++row)
    {
        for (int32 column = -7; column <= 7; ++column)
        {
            floor->AddInstance(FTransform(FRotator::ZeroRotator, FVector(row * 400, column * 400, 0), FVector(4, 4, 0.2f)));
        }
    }
    floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    //遠景にも細い幹を重ね、岩の並びを背景の壁として見せない。
    for (int32 index = 0; index < 24; ++index)
    {
        auto* distantTree = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("DistantTree%d"), index));
        distantTree->SetupAttachment(RootComponent);
        distantTree->SetStaticMesh(tree.Object);
        distantTree->SetRelativeLocation(FVector(2100 + (index % 3) * 160, index * 190 - 2200, -30));
        distantTree->SetRelativeRotation(FRotator(0, index * 53, 0));
        distantTree->SetRelativeScale3D(FVector(1.8f + (index % 4) * 0.2f));
        distantTree->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
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
        if (FVector::DistSquared2D(position, FVector(40, 60, -8)) < 14400 || FMath::Abs(position.Y) < 110) { continue; }
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
            const FVector scale = FVector(500, 480, 100 + (index % 3) * 35) / bounds.BoxExtent;
            const FVector bottom(bounds.Origin.X, bounds.Origin.Y, bounds.Origin.Z - bounds.BoxExtent.Z);
            ridge->SetRelativeScale3D(scale);
            ridge->SetRelativeLocation(FVector(2800, index * 650 - 2200, -25) - bottom * scale);
        }
    }
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> mesh(TEXT("/Game/Assets/Player/Mesh/Ch35_nonPBR"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> idle(TEXT("/Game/Assets/Player/Animation/AnimSequence/Warrior_Idle"));
    m_player = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Player"));
    m_player->SetupAttachment(RootComponent);
    m_player->SetSkeletalMesh(mesh.Object);
    m_player->SetRelativeLocation(FVector(40, 60, 0));
    //真横を向かせず、顔と胸元の両方が見える斜め正面に立たせる。
    m_player->SetRelativeRotation(FRotator(0, 120, 0));
    m_player->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    m_player->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    m_idle = idle.Object;
    //クリア時は通常の警戒姿勢から離れ、生還した喜びを一度だけ見せる。
    static ConstructorHelpers::FObjectFinder<UAnimSequence> clearAnimation(TEXT("/Game/Assets/Player/Animation/AnimSequence/Cheering"));
    m_clearAnimation = clearAnimation.Object;
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
    m_lamp->SetRelativeLocation(FVector(180, 240, 220));
    m_lamp->SetIntensity(1800);
    m_lamp->SetAttenuationRadius(1400);
    m_lamp->SetLightColor(FLinearColor(1, 0.55f, 0.26f));
    //逆光で木とプレイヤーの輪郭を拾い、暗部が黒一色になるのを防ぐ。
    auto* moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
    moon->SetupAttachment(RootComponent);
    moon->SetRelativeRotation(FRotator(-35, 135, 0));
    moon->SetIntensity(0.22f);
    //霧と半透明の描画には月光を選び、人物用の補助光との競合を防ぐ。
    moon->SetForwardShadingPriority(1);
    moon->SetLightColor(FLinearColor(0.35f, 0.50f, 0.68f));
    //顔と装備に薄い反射光を入れ、背景より人物が暗く沈むのを防ぐ。
    auto* fill = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("PlayerFill"));
    fill->SetupAttachment(RootComponent);
    fill->SetRelativeRotation(FRotator(-25, 10, 0));
    fill->SetIntensity(0.12f);
    fill->SetCastShadows(false);
    fill->SetLightColor(FLinearColor(0.65f, 0.73f, 0.85f));
    //画面右の人物だけを正面上方から照らし、森の暗さを残したまま顔と装備を読めるようにする。
    auto* portraitLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("PlayerSpotlight"));
    m_portraitLight = portraitLight;
    portraitLight->SetupAttachment(RootComponent);
    const FVector lightPosition(-250, -100, 330);
    portraitLight->SetRelativeLocation(lightPosition);
    portraitLight->SetRelativeRotation((FVector(40, 60, 70) - lightPosition).Rotation());
    //光量の単位を明示し、距離減衰で顔の照明がほぼ消える旧設定を置き換える。
    portraitLight->SetIntensityUnits(ELightUnits::Lumens);
    portraitLight->SetIntensity(550.0f);
    portraitLight->SetAttenuationRadius(850.0f);
    portraitLight->SetInnerConeAngle(32.0f);
    portraitLight->SetOuterConeAngle(48.0f);
    portraitLight->SetLightColor(FLinearColor(0.88f, 0.93f, 1.0f));
    portraitLight->SetCastShadows(false);
    auto* fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("ForestFog"));
    fog->SetupAttachment(RootComponent);
    fog->SetFogDensity(0.09f);
    fog->SetStartDistance(550.0f);
    fog->SetFogInscatteringColor(FLinearColor(0.035f, 0.055f, 0.065f));
    m_camera = CreateDefaultSubobject<UCameraComponent>(TEXT("MenuCamera"));
    m_camera->SetupAttachment(RootComponent);
    m_camera->SetRelativeLocation(m_cameraHome);
    m_camera->SetRelativeRotation((m_cameraFocus - m_cameraHome).Rotation());
    m_camera->FieldOfView = 48;
    m_camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
    m_camera->PostProcessSettings.AutoExposureMethod = AEM_Manual;
    m_camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    m_camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
    m_camera->PostProcessSettings.bOverride_AutoExposureBias = true;
    m_camera->PostProcessSettings.AutoExposureBias = 1;
    m_camera->PostProcessSettings.bOverride_VignetteIntensity = true;
    m_camera->PostProcessSettings.VignetteIntensity = 0.18f;
    //明るい人物の輪郭がにじまないよう、メニューでは強い発光と残像を使わない。
    m_camera->PostProcessSettings.bOverride_BloomIntensity = true;
    m_camera->PostProcessSettings.BloomIntensity = 0.15f;
    m_camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
    m_camera->PostProcessSettings.MotionBlurAmount = 0.0f;
    //背景全体を明るくする代わりに人物へ焦点を置き、枝葉の細かさと顔を分離する。
    m_camera->PostProcessSettings.bOverride_DepthOfFieldFstop = true;
    m_camera->PostProcessSettings.DepthOfFieldFstop = 3.2f;
    m_camera->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
}

void AGameFlowScene::SetScene(int32 _scene)
{
    m_scene = FMath::Clamp(_scene, 0, 2);
    m_time = 0.0f;
    //画面を開いた直後に人物や地面が低解像度のまま映らないよう、撮影用の素材を先読みする。
    TInlineComponentArray<UMeshComponent*> sceneMeshes(this);
    for (UMeshComponent* part : sceneMeshes) { part->PrestreamTextures(5.0f, true); }
    //開始は警戒、クリアは生還の動作、敗北は倒れる動作に分ける。
    UAnimSequence* animation = m_scene == 2 ? m_death.Get() : m_scene == 1 ? m_clearAnimation.Get() : m_idle.Get();
    const USkeletalMesh* mesh = m_player->GetSkeletalMeshAsset();
    if (!animation || !mesh || animation->GetSkeleton() != mesh->GetSkeleton()) { animation = m_idle.Get(); }
    //敗北時は倒れる動作を一度だけ再生し、完了後の姿勢を保つ。
    m_player->PlayAnimation(animation, m_scene == 0);
    m_player->SetPlayRate(m_scene == 0 ? 0.0f : 1.0f);
    UpdateIdlePose();
    m_player->SetVisibility(true);
    m_lamp->SetLightColor(m_scene == 2 ? FLinearColor(1, 0.12f, 0.06f) : FLinearColor(1, 0.55f, 0.26f));
    //敗北画面でも人物を隠さず、暖色と寒色の違いで場面を伝える。
    m_camera->PostProcessSettings.AutoExposureBias = m_scene == 1 ? 0.8f : 0.5f;
    m_portraitLight->SetLightColor(m_scene == 1 ? FLinearColor(1.0f, 0.86f, 0.66f) : FLinearColor(0.80f, 0.90f, 1.0f));
    //敗北時は低い視点で体を見せ、クリア時は腕を上げる動作まで画角に収める。
    m_cameraHome = m_scene == 2 ? FVector(-330, -240, 150) : m_scene == 1 ? FVector(-420, -210, 155) : FVector(-310, -170, 140);
    m_cameraFocus = m_scene == 2 ? FVector(40, -40, 35) : m_scene == 1 ? FVector(40, -70, 115) : FVector(40, -45, 105);
    UpdateCamera();
}

void AGameFlowScene::Tick(float _deltaTime)
{
    Super::Tick(_deltaTime);
    m_time += _deltaTime;
    UpdateIdlePose();
    UpdateCamera();
    m_lamp->SetIntensity(1800 * (m_scene == 2 ? 0.88f + 0.12f * FMath::Sin(m_time * 2.1f) : 1));
}

//呼吸部分を往復させ、ループの継ぎ目で腕や腰の位置が跳ねないようにする。
void AGameFlowScene::UpdateIdlePose()
{
    if (m_scene != 0 || !m_idle || !m_player) { return; }
    //端で速度がゼロになる曲線で、静かな警戒姿勢の中だけを移動する。
    const float phase = 0.5f - 0.5f * FMath::Cos(m_time * UE_TWO_PI / FMath::Max(1.0f, m_idleCycle));
    const float start = FMath::Clamp(m_idleStart, 0.0f, m_idle->GetPlayLength());
    const float end = FMath::Clamp(m_idleEnd, start, m_idle->GetPlayLength());
    m_player->SetPosition(FMath::Lerp(start, end, phase), false);
}

//最初の二秒だけゆっくり寄り、その後は注視点を保った小さな揺れに留める。
void AGameFlowScene::UpdateCamera()
{
    const float arrival = FMath::SmoothStep(0.0f, 2.0f, m_time);
    const FVector retreat = (m_cameraHome - m_cameraFocus).GetSafeNormal() * (1.0f - arrival) * 18.0f;
    const FVector location = m_cameraHome + retreat + FVector(0, FMath::Sin(m_time * 0.18f) * 2.0f, 0);
    m_camera->SetRelativeLocation(location);
    m_camera->SetRelativeRotation((m_cameraFocus - location).Rotation());
    //倒れた場面では地面近くへ焦点を下げ、手前の草だけが鮮明になるのを避ける。
    const FVector subject(40, 60, m_scene == 2 ? 30.0f : 120.0f);
    m_camera->PostProcessSettings.DepthOfFieldFocalDistance = FVector::Distance(location, subject);
}
