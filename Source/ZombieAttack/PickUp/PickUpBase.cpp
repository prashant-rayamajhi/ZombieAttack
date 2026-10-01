#include "PickUpBase.h"
#include "../Player/PlayerChara.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

//APickUpBaseが使用するComponentと初期パラメータを設定します。
APickUpBase::APickUpBase()
    : m_pMeshComp(nullptr), m_pSphereComp(nullptr), m_pItemIconComp(nullptr), m_pItemLightComp(nullptr), m_itemType(EItemType::EIT_Other),
      m_itemValue(0.f), m_pHealthMesh(nullptr), m_pAmmoMesh(nullptr), m_pARAmmoMesh(nullptr), m_pARWeaponMesh(nullptr), m_pHealthIcon(nullptr),
      m_pAmmoIcon(nullptr), m_floatHeight(40.f), m_arWeaponFloatHeightAdjustment(-25.f), m_floatAmplitude(15.f), m_floatSpeed(2.f),
      m_rotateSpeed(90.f), m_spawnLocation(FVector::ZeroVector), m_floatTime(0.f), m_bConsumed(false)
{
    PrimaryActorTick.bCanEverTick = true;

    m_pSphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
    SetRootComponent(m_pSphereComp);
    m_pSphereComp->InitSphereRadius(70.f);
    m_pSphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    m_pSphereComp->SetCollisionObjectType(ECC_WorldDynamic);
    m_pSphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    m_pSphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    m_pSphereComp->SetGenerateOverlapEvents(true);
    m_pSphereComp->OnComponentBeginOverlap.AddDynamic(this, &APickUpBase::OnOverlapBegin);

    m_pMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    m_pMeshComp->SetupAttachment(m_pSphereComp);
    m_pMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    //種類を遠くから判別できる、常にカメラを向く発光アイコン
    m_pItemIconComp = CreateDefaultSubobject<UBillboardComponent>(TEXT("PickupTypeIcon"));
    m_pItemIconComp->SetupAttachment(m_pSphereComp);
    m_pItemIconComp->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
    m_pItemIconComp->SetRelativeScale3D(FVector(0.22f));
    m_pItemIconComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    m_pItemLightComp = CreateDefaultSubobject<UPointLightComponent>(TEXT("PickupGlow"));
    m_pItemLightComp->SetupAttachment(m_pSphereComp);
    m_pItemLightComp->SetRelativeLocation(FVector(0.0f, 0.0f, 55.0f));
    m_pItemLightComp->SetIntensity(1800.0f);
    m_pItemLightComp->SetAttenuationRadius(280.0f);
    m_pItemLightComp->SetCastShadows(false);
    static ConstructorHelpers::FObjectFinder<UTexture2D> HealthIcon(TEXT("/Game/UI/PickupIcons/T_HealthPickup.T_HealthPickup"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> AmmoIcon(TEXT("/Game/UI/PickupIcons/T_AmmoPickup.T_AmmoPickup"));
    m_pHealthIcon = HealthIcon.Object;
    m_pAmmoIcon = AmmoIcon.Object;
}

//ゲーム開始時の初期設定を行います。
void APickUpBase::BeginPlay()
{
    //ゲーム開始時に必要な参照を取得し、初期状態を整えます。
    Super::BeginPlay();
    m_spawnLocation = GetActorLocation();
    ApplyMeshByType();
}

//毎フレームの更新を行います。
void APickUpBase::Tick(float _deltaTime)
{
    //フレームごとの経過時間を使って、移動や表示の変化を更新します。
    Super::Tick(_deltaTime);
    m_floatTime += _deltaTime;
    const float itemHeightAdjustment = m_itemType == EItemType::EIT_WeaponAR ? m_arWeaponFloatHeightAdjustment : 0.0f;
    const float ZOffset = m_floatHeight + itemHeightAdjustment + FMath::Sin(m_floatTime * m_floatSpeed) * m_floatAmplitude;
    SetActorLocation(m_spawnLocation + FVector(0.f, 0.f, ZOffset));
    AddActorWorldRotation(FRotator(0.f, m_rotateSpeed * _deltaTime, 0.f));
    const float pulse = 0.5f + 0.5f * FMath::Sin(m_floatTime * 3.2f);
    if (m_pItemIconComp)
    {
        m_pItemIconComp->SetRelativeScale3D(FVector(FMath::Lerp(0.19f, 0.235f, pulse)));
    }
    if (m_pItemLightComp)
    {
        m_pItemLightComp->SetIntensity(FMath::Lerp(1100.0f, 2200.0f, pulse));
    }
}

//アイテムを取得し、効果を反映します。
void APickUpBase::PickUpItem(EItemType _itemType, float _amount)
{
    m_itemType = _itemType;
    m_itemValue = _amount;
    ApplyMeshByType();
}

//PickupPresentationを所有しているか判定します。
bool APickUpBase::HasPickupPresentation() const { return IsValid(m_pItemIconComp) && IsValid(m_pItemIconComp->Sprite) && IsValid(m_pItemLightComp); }

//MeshByTypeを対象へ適用します。
void APickUpBase::ApplyMeshByType()
{
    if (!m_pMeshComp) { return; }

    //対象メッシュを保持します。
    UStaticMesh* TargetMesh = nullptr;
    UTexture2D* iconTexture = nullptr;
    FLinearColor glowColor = FLinearColor(1.0f, 0.45f, 0.04f, 1.0f);
    switch (m_itemType)
    {
    case EItemType::EIT_Health:
        TargetMesh = m_pHealthMesh;
        iconTexture = m_pHealthIcon;
        glowColor = FLinearColor(0.08f, 1.0f, 0.22f, 1.0f);
        break;
    case EItemType::EIT_Ammo:
        TargetMesh = m_pAmmoMesh;
        iconTexture = m_pAmmoIcon;
        break;
    case EItemType::EIT_ARAmmo:
        TargetMesh = m_pARAmmoMesh ? m_pARAmmoMesh.Get() : m_pAmmoMesh.Get();
        iconTexture = m_pAmmoIcon;
        break;
    case EItemType::EIT_WeaponAR:
        //対象メッシュを表す項目
        TargetMesh = m_pARWeaponMesh ? m_pARWeaponMesh.Get() : (m_pARAmmoMesh ? m_pARAmmoMesh.Get() : m_pAmmoMesh.Get());
        if (!TargetMesh)
        {
            TargetMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Assets/Weapon/Mesh/SM_ARRifle.SM_ARRifle"));
        }
        iconTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/WeaponIcons/T_AR_Icon.T_AR_Icon"));
        glowColor = FLinearColor(0.05f, 0.55f, 1.0f, 1.0f);
        break;
    default: break;
    }

    m_pMeshComp->SetStaticMesh(TargetMesh);
    if (m_pItemIconComp)
    {
        m_pItemIconComp->SetSprite(iconTexture);
        m_pItemIconComp->SetVisibility(iconTexture != nullptr);
    }
    if (m_pItemLightComp)
    {
        m_pItemLightComp->SetLightColor(glowColor);
    }
    if (TargetMesh && m_itemType == EItemType::EIT_WeaponAR)
    {
        m_pMeshComp->SetRelativeScale3D(FVector(0.7f));
        m_pMeshComp->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    }
}

//OverlapBeginが発生したときの処理を行います。
void APickUpBase::OnOverlapBegin(UPrimitiveComponent* _pOverlappedComp, AActor* _pOtherActor, UPrimitiveComponent* _pOtherComp,
                                 int32 _otherBodyIndex, bool _bFromSweep, const FHitResult& _sweepResult)
{
    if (m_bConsumed) { return; }
    if (APlayerChara* Player = Cast<APlayerChara>(_pOtherActor))
    {
        if (!Player->PickUpItem(m_itemValue, m_itemType)) { return; }

        m_bConsumed = true;
        m_pSphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Destroy();
    }
}
