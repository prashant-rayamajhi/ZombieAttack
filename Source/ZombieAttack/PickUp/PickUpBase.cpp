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

    //HealthIconは、HealthIconの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    static ConstructorHelpers::FObjectFinder<UTexture2D> HealthIcon(TEXT("/Game/UI/PickupIcons/T_HealthPickup.T_HealthPickup"));
    //AmmoIconは、AmmoIconの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
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
void APickUpBase::Tick(float DeltaTime)
{
    //フレームごとの経過時間を使って、移動や表示の変化を更新します。
    Super::Tick(DeltaTime);
    m_floatTime += DeltaTime;

    //itemHeightAdjustmentは、m_itemType == EItemType::EIT_WeaponAR ? m_arWeaponFloatHeightAdjustment…から算出した数値を後続の判定または計算に使います。
    const float itemHeightAdjustment = m_itemType == EItemType::EIT_WeaponAR ? m_arWeaponFloatHeightAdjustment : 0.0f;
    //ZOffsetは、m_floatHeight + itemHeightAdjustment + FMath::Sin(m_floatTime * m_float…から算出した数値を後続の判定または計算に使います。
    const float ZOffset = m_floatHeight + itemHeightAdjustment + FMath::Sin(m_floatTime * m_floatSpeed) * m_floatAmplitude;
    SetActorLocation(m_spawnLocation + FVector(0.f, 0.f, ZOffset));
    AddActorWorldRotation(FRotator(0.f, m_rotateSpeed * DeltaTime, 0.f));

    //pulseは、0.5f + 0.5f * FMath::Sin(m_floatTime * 3.2f)から算出した数値を後続の判定または計算に使います。
    const float pulse = 0.5f + 0.5f * FMath::Sin(m_floatTime * 3.2f);
    //「m_pItemIconComp」が成立するとき、SetRelativeScale3Dを呼び出します。
    if (m_pItemIconComp)
    {
        m_pItemIconComp->SetRelativeScale3D(FVector(FMath::Lerp(0.19f, 0.235f, pulse)));
    }
    //「m_pItemLightComp」が成立するとき、SetIntensityを呼び出します。
    if (m_pItemLightComp)
    {
        m_pItemLightComp->SetIntensity(FMath::Lerp(1100.0f, 2200.0f, pulse));
    }
}

//アイテムを取得し、効果を反映します。
void APickUpBase::PickUpItem(EItemType Type, float Value)
{
    m_itemType = Type;
    m_itemValue = Value;
    ApplyMeshByType();
}

//PickupPresentationを所有しているか判定します。
bool APickUpBase::HasPickupPresentation() const { return IsValid(m_pItemIconComp) && IsValid(m_pItemIconComp->Sprite) && IsValid(m_pItemLightComp); }

//MeshByTypeを対象へ適用します。
void APickUpBase::ApplyMeshByType()
{
    //「!m_pMeshComp」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (!m_pMeshComp) { return; }

    //対象メッシュを保持します。
    UStaticMesh* TargetMesh = nullptr;
    //iconTextureは、nullptrから取得した参照を後続の呼び出しで使います。
    UTexture2D* iconTexture = nullptr;
    //glowColorは、FLinearColor(1.0f, 0.45f, 0.04f, 1.0f)から構築した結果を後続の処理へ渡すために使います。
    FLinearColor glowColor = FLinearColor(1.0f, 0.45f, 0.04f, 1.0f);
    //現在の状態に合う処理へ分けます。
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
        //「!TargetMesh」が成立するとき、TargetMeshを更新します。
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
    //「m_pItemIconComp」が成立するとき、SetSpriteを呼び出します。
    if (m_pItemIconComp)
    {
        m_pItemIconComp->SetSprite(iconTexture);
        m_pItemIconComp->SetVisibility(iconTexture != nullptr);
    }
    //「m_pItemLightComp」が成立するとき、SetLightColorを呼び出します。
    if (m_pItemLightComp)
    {
        m_pItemLightComp->SetLightColor(glowColor);
    }
    //「TargetMesh && m_itemType == EItemType::EIT_WeaponAR」が成立するとき、SetRelativeScale3Dを呼び出します。
    if (TargetMesh && m_itemType == EItemType::EIT_WeaponAR)
    {
        m_pMeshComp->SetRelativeScale3D(FVector(0.7f));
        m_pMeshComp->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    }
}

//OverlapBeginが発生したときの処理を行います。
void APickUpBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
                                 int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    //「m_bConsumed」が成立するとき、続けて「APlayerChara* Player = Cast<APlayerChara>(OtherActor)」を判定します。
    if (m_bConsumed) { return; }

    //「APlayerChara* Player = Cast<APlayerChara>(OtherActor)」が成立するとき、続けて「!Player->PickUpItem(m_itemValue, m_itemType)」を判定します。
    if (APlayerChara* Player = Cast<APlayerChara>(OtherActor))
    {
        //「!Player->PickUpItem(m_itemValue, m_itemType)」が成立するとき、m_bConsumedを更新します。
        if (!Player->PickUpItem(m_itemValue, m_itemType)) { return; }

        m_bConsumed = true;
        m_pSphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Destroy();
    }
}
