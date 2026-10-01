#include "PlayerChara.h"
#include "../Weapon/ARWeapon.h"
#include "../Weapon/GunWeapon.h"
#include "../Weapon/MeleeWeapon.h"
#include "../Weapon/WeaponBase.h"
#include "../Components/PlayerAudio/PlayerAudioComponent.h"
#include "../Components/RifleAnimation/PlayerRifleAnimationComponent.h"
#include "../UI/Ammo/AmmoHUDWidget.h"
#include "../UI/PlayerUI/PlayerHP.h"
#include "../UI/PlayerUI/CombatCrosshairWidget.h"
#include "../UI/PlayerUI/WeaponCarouselWidget.h"
#include "../UI/EnemyUI/EnemyLocatorWidget.h"
#include "../UI/Configuration/ZombieAttackUISettings.h"
#include "Animation/AnimSequenceBase.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

//プレイヤーキャラクターを処理します。
APlayerChara::APlayerChara()
    : m_pPistolReloadAnimation(nullptr), m_pRifleReloadAnimation(nullptr), m_pLandingMontage(nullptr), m_idleTimeoutSeconds(8.f),
      m_reloadSpeedScale(0.4f), m_switchWeaponSpeedScale(1.0f),
      m_jumpStartMoveLockTime(0.12f), m_landingFallbackUnlockTime(0.45f), m_pSpringArm(nullptr), m_pCamera(nullptr), m_pPlayerFillLight(nullptr),
      m_pAudioComponent(nullptr), m_pRifleAnimationComponent(nullptr), m_cameraPitchLimit(FVector2D(-80.f, 80.f)),
      m_moveSpeed(600.f), m_gravity(980.f), m_jumpPower(450.f),
      m_weaponSocketName(TEXT("WeaponSocket")), m_defaultFOV(90.f), m_aimFOV(60.f), m_aimInterpSpeed(10.f),
      m_defaultCameraSocketOffset(0.f, 100.f, 80.f), m_rifleAimCameraSocketOffset(0.f, 52.f, 76.f), m_defaultCameraArmLength(200.f),
      m_rifleAimCameraArmLength(235.f), m_aimCameraPositionInterpSpeed(9.f), m_rifleAimInwardYawOffset(0.f), m_rifleVisualConvergenceDistance(350.f),
      m_pUserWidget(nullptr), m_pPlayerHp(nullptr), m_pReloadUI(nullptr), m_pWeaponCarousel(nullptr), m_pEnemyLocatorWidget(nullptr),
      m_pPistolWeapon(nullptr), m_pKnifeWeapon(nullptr), m_pARWeapon(nullptr), m_currentSlot(EWeaponSlot::Pistol),
      //待機状態を処理します。
      m_idleState(EIdleState::InitialIdle), m_healItemCount(0), m_crosshairMaxDist(10000.f),
      //キャラクター移動を処理します。
      m_charaMovement(FVector2D::ZeroVector), m_cameraRotation(FVector2D::ZeroVector), m_defaultMeshRelativeRotation(FRotator::ZeroRotator),
      m_idleTimerAccum(0.f), m_bJumping(false),
      m_bCanControl(true), m_bIsAiming(false), m_bIsFiringRifle(false), m_bRifleCombatAim(false), m_bPendingRifleShot(false),
      m_bRifleTriggerHeld(false), m_bIsHealing(false), m_bHasAR(false), m_bIsReloadingAnim(false), m_bIsSwitchingWeapon(false),
      m_bMovementLockedByAnimation(false), m_bCrosshairSuppressed(false), m_bIsDead(false)
{
    PrimaryActorTick.bCanEverTick = true;

    //本編ではGameModeがPawnの生成とPossessを担当します。
    //Pawn側のAutoPossessを併用すると入力Componentが二重初期化されるため無効にします。
    AutoPossessPlayer = EAutoReceiveInput::Disabled;

    //カメラコンポーネントの生成と設定
    m_pSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    if (m_pSpringArm)
    {
        m_pSpringArm->SetupAttachment(RootComponent);
        m_pSpringArm->TargetArmLength = m_defaultCameraArmLength;
        m_pSpringArm->bDoCollisionTest = false;
        m_pSpringArm->bEnableCameraLag = true;
        m_pSpringArm->CameraLagSpeed = 10.f;
        m_pSpringArm->bEnableCameraRotationLag = true;
        m_pSpringArm->CameraRotationLagSpeed = 10.f;
        m_pSpringArm->bUseCameraLagSubstepping = true;
        m_pSpringArm->CameraLagMaxTimeStep = 1.0f / 60.0f;
        m_pSpringArm->SocketOffset = m_defaultCameraSocketOffset;
    }

    m_pCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    if (m_pCamera && m_pSpringArm)
    {
        m_pCamera->SetupAttachment(m_pSpringArm, USpringArmComponent::SocketName);

        //露出はレベルのPostProcessVolumeで一元管理し、カメラとの二重補正を避けます。
        m_pCamera->PostProcessBlendWeight = 1.0f;
        m_pCamera->PostProcessSettings.bOverride_AutoExposureBias = false;

        //画面端を締めつつ、暗部を隠しすぎない弱いビネットに抑えます。
        m_pCamera->PostProcessSettings.bOverride_VignetteIntensity = true;
        m_pCamera->PostProcessSettings.VignetteIntensity = 0.22f;
    }

    //プレイヤーの右前方から弱い青白色を当て、暗い背景でも輪郭を判別可能にします。
    m_pPlayerFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PlayerFillLight"));
    m_pPlayerFillLight->SetupAttachment(RootComponent);
    m_pPlayerFillLight->SetRelativeLocation(FVector(45.0f, 55.0f, 135.0f));
    m_pPlayerFillLight->SetIntensity(220.0f);
    m_pPlayerFillLight->SetAttenuationRadius(260.0f);
    m_pPlayerFillLight->SetLightColor(FLinearColor(0.68f, 0.78f, 1.0f));
    m_pPlayerFillLight->SetCastShadows(false);
    m_pPlayerFillLight->SetVolumetricScatteringIntensity(0.15f);

    m_pAudioComponent = CreateDefaultSubobject<UPlayerAudioComponent>(TEXT("PlayerAudioComponent"));
    m_pRifleAnimationComponent = CreateDefaultSubobject<UPlayerRifleAnimationComponent>(TEXT("PlayerRifleAnimationComponent"));
    static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> PistolReload(TEXT("/Game/Assets/Player/Animation/PistolReloading.PistolReloading"));
    //ライフルリロードを処理します。
    static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> RifleReload(
        TEXT("/Game/Assets/Player/Animation/AnimSequence/RifleReloading.RifleReloading"));
    m_pPistolReloadAnimation = PistolReload.Object;
    m_pRifleReloadAnimation = RifleReload.Object;
}

//ゲーム開始時の初期設定を行います。
void APlayerChara::BeginPlay()
{
    //ゲーム開始時に必要な参照を取得し、初期状態を整えます。
    Super::BeginPlay();
    if (GetMesh())
    {
        m_defaultMeshRelativeRotation = GetMesh()->GetRelativeRotation();
    }
    if (m_pRifleAnimationComponent)
    {
        m_pRifleAnimationComponent->OnAimReady().AddUObject(this, &APlayerChara::HandleRifleAimReady);
    }
    if (auto* mv = GetCharacterMovement())
    {
        mv->AirControl = 0.8f;
        mv->MaxWalkSpeed = m_moveSpeed;
        mv->JumpZVelocity = m_jumpPower;
        mv->GravityScale = m_gravity / 980.f;
        //ストレーフのために移動方向自動回転を無効化（UpdateMoveで手動制御）
        mv->bOrientRotationToMovement = false;
        mv->bUseControllerDesiredRotation = false;
    }

    //ピストル生成
    if (m_defaultWeapon && GetWorld())
    {
        FActorSpawnParameters spawnParams;
        spawnParams.Owner = this;
        spawnParams.Instigator = GetInstigator();
        //ワールドを返します。
        AWeaponBase* pistol = GetWorld()->SpawnActor<AWeaponBase>(m_defaultWeapon, FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);
        if (pistol)
        {
            pistol->SetOwnerCharacter(this);
            pistol->SetOwner(this);
            if (auto* mesh = GetMesh())
            {
                FAttachmentTransformRules attachRules(EAttachmentRule::SnapToTarget, true);
                pistol->AttachToComponent(mesh, attachRules, m_weaponSocketName);
            }
            m_pCurrentWeapon = pistol;
            m_pPistolWeapon = pistol;
            m_pEquippedWeapon = pistol;
        }
    }

    //ナイフ生成（非表示待機）
    if (m_meleeWeaponClass)
    {
        FActorSpawnParameters spawnParams;
        spawnParams.Owner = this;
        spawnParams.Instigator = GetInstigator();
        //ワールドを返します。
        AMeleeWeapon* knife = GetWorld()->SpawnActor<AMeleeWeapon>(m_meleeWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);
        if (knife)
        {
            knife->SetOwnerCharacter(this);
            knife->SetOwner(this);
            if (auto* mesh = GetMesh())
            {
                FAttachmentTransformRules attachRules(EAttachmentRule::SnapToTarget, true);
                knife->AttachToComponent(mesh, attachRules, TEXT("KnifeSocket"));
            }
            m_pKnifeWeapon = knife;
            knife->SetActorHiddenInGame(true);
        }
    }

    //BPでクラスが未設定でも、C++製のHP HUDを必ず表示します。
    const UZombieAttackUISettings* UISettings = GetDefault<UZombieAttackUISettings>();
    TSubclassOf<UUserWidget> HealthWidgetClass = UISettings->GetPlayerHealthWidgetClass();
    if (!HealthWidgetClass)
    {
        HealthWidgetClass = m_playerHPClass;
        if (!HealthWidgetClass)
        {
            HealthWidgetClass = UPlayerHP::StaticClass();
        }
    }
    if (HealthWidgetClass)
    {
        m_pPlayerHp = CreateWidget<UUserWidget>(GetWorld(), HealthWidgetClass);
        if (m_pPlayerHp)
        {
            m_pPlayerHp->AddToViewport(10);
            m_pPlayerHp->SetVisibility(ESlateVisibility::HitTestInvisible);
            if (UPlayerHP* hp = Cast<UPlayerHP>(m_pPlayerHp))
            {
                hp->SetOwner(this);
                m_onDamaged.AddDynamic(hp, &UPlayerHP::UpdateHealthUI);
                hp->UpdateHealthUI();
            }
        }
    }

    //リロード UI
    m_pReloadUI = CreateWidget<UAmmoHUDWidget>(GetWorld(), UISettings->GetAmmoWidgetClass());
    if (m_pReloadUI)
    {
        m_pReloadUI->AddToViewport(15);
        m_pReloadUI->SetWeapon(Cast<AGunWeapon>(m_pCurrentWeapon));
    }

    //通常・エイム・撃破確認を単一Widgetで描画し、二重クロスヘアを防ぎます。
    m_pUserWidget = CreateWidget<UCombatCrosshairWidget>(GetWorld(), UISettings->GetCrosshairWidgetClass());
    if (m_pUserWidget)
    {
        m_pUserWidget->AddToViewport(30);
        //表示状態を設定します。
        m_pUserWidget->SetVisibility(m_bCrosshairSuppressed ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }

    //アウトライン対象のうち最寄りの敵を、回転する赤い方向矢印で案内します。
    m_pEnemyLocatorWidget = CreateWidget<UEnemyLocatorWidget>(GetWorld(), UISettings->GetEnemyLocatorWidgetClass());
    if (m_pEnemyLocatorWidget)
    {
        m_pEnemyLocatorWidget->AddToViewport(29);
        m_pEnemyLocatorWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    if (m_weaponCarouselClass)
    {
        m_pWeaponCarousel = CreateWidget<UWeaponCarouselWidget>(GetWorld(), m_weaponCarouselClass);
        if (m_pWeaponCarousel)
        {
            //武器切り替えUIはHUDより前面に出します。
            m_pWeaponCarousel->AddToViewport(20);
            m_pWeaponCarousel->SetAnchorsInViewport(FAnchors(0.0f, 0.0f));
            m_pWeaponCarousel->SetAlignmentInViewport(FVector2D::ZeroVector);
            m_pWeaponCarousel->SetPositionInViewport(FVector2D(32.0f, 158.0f), false);
            m_pWeaponCarousel->SetDesiredSizeInViewport(FVector2D(360.0f, 300.0f));
            m_pWeaponCarousel->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }
    RebuildWeaponDisplayOrder();
    RefreshWeaponCarousel();
    if (m_pCamera)
    {
        m_pCamera->SetFieldOfView(m_defaultFOV);
        if (UMaterialInterface* outlineMaterial =
                LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Assets/Material/M_PP_EnemyOutline.M_PP_EnemyOutline")))
        {
            m_pCamera->PostProcessSettings.AddBlendable(outlineMaterial, 1.0f);
        }
    }

    ControlHUDVisiblity();
}

//毎フレームの更新を行います。
void APlayerChara::Tick(float _deltaTime)
{
    //フレームごとの経過時間を使って、移動や表示の変化を更新します。
    Super::Tick(_deltaTime);
    if (m_bIsDead) { return; }

    //各種更新処理
    //撃破の一瞬だけ世界を遅くしても、照準を動かす手応えは変えない。
    const float cameraDelta = m_killStopActive ? _deltaTime / FMath::Clamp(m_killHitStopTimeDilation, 0.01f, 1.0f) : _deltaTime;
    UpdateCamera(cameraDelta);
    UpdateMove(_deltaTime);
    UpdateJump(_deltaTime);
    UpdateCrosshair(_deltaTime);
    UpdateIdleState(_deltaTime);
    UpdateBufferedAttack();

    //FOV補間
    if (m_pCamera)
    {
        //エイム中かどうかを返します。
        const float target = IsAiming() ? m_aimFOV : m_defaultFOV;
        m_pCamera->SetFieldOfView(FMath::FInterpTo(m_pCamera->FieldOfView, target, _deltaTime, m_aimInterpSpeed));
    }
    if (m_pSpringArm)
    {
        //エイム中かどうかを返します。
        const bool bRifleAimCamera = m_currentSlot == EWeaponSlot::AR && IsAiming();
        const FVector targetOffset = bRifleAimCamera ? m_rifleAimCameraSocketOffset : m_defaultCameraSocketOffset;
        const float targetArmLength = bRifleAimCamera ? m_rifleAimCameraArmLength : m_defaultCameraArmLength;
        m_pSpringArm->SocketOffset = FMath::VInterpTo(m_pSpringArm->SocketOffset, targetOffset, _deltaTime, m_aimCameraPositionInterpSpeed);
        m_pSpringArm->TargetArmLength = FMath::FInterpTo(m_pSpringArm->TargetArmLength, targetArmLength, _deltaTime, m_aimCameraPositionInterpSpeed);
    }

    //ストレーフ中は自動回転を常に無効
    GetCharacterMovement()->bOrientRotationToMovement = false;
}

//入力バインド

void APlayerChara::SetupPlayerInputComponent(UInputComponent* _pInputComp)
{
    //親クラスの入力設定を先に適用する
    Super::SetupPlayerInputComponent(_pInputComp);

    //軸入力とアクション入力のバインド
    _pInputComp->BindAxis("MoveForward", this, &APlayerChara::Chara_MoveForward);
    _pInputComp->BindAxis("MoveRight", this, &APlayerChara::Chara_MoveRight);
    _pInputComp->BindAxis("CameraPitch", this, &APlayerChara::Cam_RotatePitch);
    _pInputComp->BindAxis("CameraYaw", this, &APlayerChara::Cam_RotateYaw);
    _pInputComp->BindAxis("WeaponWheel", this, &APlayerChara::OnWeaponWheel);

    //ジャンプ、攻撃、エイム、リロード、回復アイテム使用、武器切り替えのアクションバインド
    _pInputComp->BindAction("Jump", IE_Pressed, this, &APlayerChara::JumpStart);
    _pInputComp->BindAction("Fire", IE_Pressed, this, &APlayerChara::Attack);
    _pInputComp->BindAction("Fire", IE_Released, this, &APlayerChara::StopAttack);
    _pInputComp->BindAction("Aim", IE_Pressed, this, &APlayerChara::StartAim);
    _pInputComp->BindAction("Aim", IE_Released, this, &APlayerChara::StopAim);
    _pInputComp->BindAction("Reload", IE_Pressed, this, &APlayerChara::ReloadWeapon);
    _pInputComp->BindAction("Heal", IE_Pressed, this, &APlayerChara::UseHealItem);
    _pInputComp->BindAction("Slot1", IE_Pressed, this, &APlayerChara::SwitchToPistol);
    _pInputComp->BindAction("Slot2", IE_Pressed, this, &APlayerChara::SwitchToAR);
    _pInputComp->BindAction("Slot3", IE_Pressed, this, &APlayerChara::SwitchToKnife);
    _pInputComp->BindAction("NextWeapon", IE_Pressed, this, &APlayerChara::CycleNextWeapon);
    _pInputComp->BindAction("PreviousWeapon", IE_Pressed, this, &APlayerChara::CyclePreviousWeapon);

}
