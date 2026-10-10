#include "EnemyHitFeedbackComponent.h"

#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float BloodDecalDepth = 8.0f;
constexpr float BloodDecalSurfaceOffset = 1.5f;
constexpr float BloodDropletSurfaceOffset = 3.0f;
constexpr float BloodDropletConeDegrees = 38.0f;
//大粒の球に見えない細かな飛沫で命中点を伝える。
constexpr float BloodDropletMinScale = 0.008f;
constexpr float BloodDropletMaxScale = 0.018f;
constexpr float BloodSpriteSurfaceOffset = 2.0f;
constexpr float BloodSpriteStartScaleMultiplier = 0.72f;
constexpr float BloodSpriteEndScaleMultiplier = 1.08f;
//名前空間を閉じます。
//名前空間を閉じます。
}

//UEnemyHitFeedbackComponentが使用するComponentと初期パラメータを設定します。
UEnemyHitFeedbackComponent::UEnemyHitFeedbackComponent()
    : m_pBloodDecalMaterial(nullptr), m_pBloodDropletMaterial(nullptr), m_pBloodImpactSpriteMaterial(nullptr), m_pDropletMesh(nullptr),
      m_pImpactPlaneMesh(nullptr), m_dropletCount(10), m_decalSize(24.0f), m_decalLifeSeconds(12.0f), m_dropletLifeSeconds(0.55f),
      m_dropletSpeedMin(150.0f), m_dropletSpeedMax(280.0f), m_dropletGravity(620.0f), m_impactSpriteLifeSeconds(0.26f), m_impactSpriteScale(0.34f)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> bloodDecalMaterial(
        TEXT("/Game/VFX/Blood/Materials/M_BloodImpactDecal.M_BloodImpactDecal"));
    if (bloodDecalMaterial.Succeeded())
    {
        m_pBloodDecalMaterial = bloodDecalMaterial.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> bloodDropletMaterial(
        TEXT("/Game/VFX/Blood/Materials/M_BloodDroplet.M_BloodDroplet"));
    if (bloodDropletMaterial.Succeeded())
    {
        m_pBloodDropletMaterial = bloodDropletMaterial.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> bloodImpactSpriteMaterial(
        TEXT("/Game/VFX/Blood/Materials/M_BloodImpactSprite.M_BloodImpactSprite"));
    if (bloodImpactSpriteMaterial.Succeeded())
    {
        m_pBloodImpactSpriteMaterial = bloodImpactSpriteMaterial.Object;
    }
    static ConstructorHelpers::FObjectFinder<UStaticMesh> dropletMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (dropletMesh.Succeeded())
    {
        m_pDropletMesh = dropletMesh.Object;
    }
    static ConstructorHelpers::FObjectFinder<UStaticMesh> impactPlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
    if (impactPlaneMesh.Succeeded())
    {
        m_pImpactPlaneMesh = impactPlaneMesh.Object;
    }
}

//所有者の状態変化をComponentから毎フレーム更新します。
void UEnemyHitFeedbackComponent::TickComponent(float _deltaTime, ELevelTick _tickType, FActorComponentTickFunction* _thisTickFunction)
{
    Super::TickComponent(_deltaTime, _tickType, _thisTickFunction);

    //「int32 i = m_activeBloodDroplets.Num() - 1; i >= 0; --i」で列挙される各要素へ、ループ本体の判定と更新を適用します。
    for (int32 i = m_activeBloodDroplets.Num() - 1; i >= 0; --i)
    {
        FActiveBloodDroplet& droplet = m_activeBloodDroplets[i];
        UStaticMeshComponent* dropletComponent = droplet.m_pComponent.Get();
        if (!IsValid(dropletComponent))
        {
            m_activeBloodDroplets.RemoveAtSwap(i);
            continue;
        }

        droplet.m_remainingLife -= _deltaTime;
        if (droplet.m_remainingLife <= 0.0f)
        {
            dropletComponent->DestroyComponent();
            m_activeBloodDroplets.RemoveAtSwap(i);
            continue;
        }

        droplet.m_velocity.Z -= m_dropletGravity * _deltaTime;
        dropletComponent->AddWorldOffset(droplet.m_velocity * _deltaTime, false);
        const float lifeAlpha = FMath::Clamp(droplet.m_remainingLife / droplet.m_totalLife, 0.0f, 1.0f);
        dropletComponent->SetWorldScale3D(droplet.m_initialScale * lifeAlpha);
    }

    //「int32 i = m_activeBloodSprites.Num() - 1; i >= 0; --i」で列挙される各要素へ、ループ本体の判定と更新を適用します。
    for (int32 i = m_activeBloodSprites.Num() - 1; i >= 0; --i)
    {
        FActiveBloodSprite& sprite = m_activeBloodSprites[i];
        UStaticMeshComponent* spriteComponent = sprite.m_pComponent.Get();
        if (!IsValid(spriteComponent))
        {
            m_activeBloodSprites.RemoveAtSwap(i);
            continue;
        }

        sprite.m_remainingLife -= _deltaTime;
        if (sprite.m_remainingLife <= 0.0f)
        {
            spriteComponent->DestroyComponent();
            m_activeBloodSprites.RemoveAtSwap(i);
            continue;
        }
        const float elapsedAlpha = FMath::Clamp(1.0f - sprite.m_remainingLife / sprite.m_totalLife, 0.0f, 1.0f);
        spriteComponent->SetWorldScale3D(sprite.m_initialScale *
                                         FMath::Lerp(BloodSpriteStartScaleMultiplier, BloodSpriteEndScaleMultiplier, elapsedAlpha));
    }
}

//BulletImpactを再生します。
void UEnemyHitFeedbackComponent::PlayBulletImpact(const FHitResult& _hitResult, const FVector& _shotDirection)
{
    if (!_hitResult.bBlockingHit) { return; }

    SpawnBloodDecal(_hitResult);
    SpawnBloodImpactSprite(_hitResult, _shotDirection);
    SpawnBloodDroplets(_hitResult, _shotDirection);
}

//BloodImpactSpriteを作成します。
void UEnemyHitFeedbackComponent::SpawnBloodImpactSprite(const FHitResult& _hitResult, const FVector& _shotDirection)
{
    //所有者を返します。
    AActor* owner = GetOwner();
    //ワールドを返します。
    UWorld* world = GetWorld();
    if (!IsValid(owner) || !world || !m_pImpactPlaneMesh || !m_pBloodImpactSpriteMaterial) { return; }
    UStaticMeshComponent* spriteComponent = NewObject<UStaticMeshComponent>(owner);
    if (!spriteComponent) { return; }
    FVector facingDirection = -_shotDirection.GetSafeNormal();
    if (facingDirection.IsNearlyZero())
    {
        facingDirection = _hitResult.ImpactNormal;
    }

    spriteComponent->SetStaticMesh(m_pImpactPlaneMesh);
    spriteComponent->SetMaterial(0, m_pBloodImpactSpriteMaterial);
    spriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    spriteComponent->SetCastShadow(false);
    spriteComponent->RegisterComponentWithWorld(world);
    spriteComponent->SetWorldLocation(_hitResult.ImpactPoint + facingDirection * BloodSpriteSurfaceOffset);
    spriteComponent->SetWorldRotation(FRotationMatrix::MakeFromZ(facingDirection).Rotator());
    const float size = FMath::Min(m_impactSpriteScale, 0.22f);
    spriteComponent->SetWorldScale3D(FVector(size));
    FActiveBloodSprite& sprite = m_activeBloodSprites.AddDefaulted_GetRef();
    sprite.m_pComponent = spriteComponent;
    sprite.m_initialScale = FVector(size);
    //連射でも赤い板が重なり続けないよう、命中した瞬間だけ表示する。
    sprite.m_remainingLife = FMath::Min(m_impactSpriteLifeSeconds, 0.16f);
    sprite.m_totalLife = sprite.m_remainingLife;
}

//BloodDecalを作成します。
void UEnemyHitFeedbackComponent::SpawnBloodDecal(const FHitResult& _hitResult) const
{
    //コンポーネントを返します。
    UPrimitiveComponent* hitComponent = _hitResult.GetComponent();
    if (!m_pBloodDecalMaterial || !IsValid(hitComponent)) { return; }
    FRotator decalRotation = _hitResult.ImpactNormal.Rotation();
    decalRotation.Roll = FMath::FRandRange(0.0f, 360.0f);

    //DecalAttachedを作成します。
    UGameplayStatics::SpawnDecalAttached(m_pBloodDecalMaterial, FVector(BloodDecalDepth, m_decalSize, m_decalSize), hitComponent, NAME_None,
                                         _hitResult.ImpactPoint + _hitResult.ImpactNormal * BloodDecalSurfaceOffset, decalRotation,
                                         EAttachLocation::KeepWorldPosition, m_decalLifeSeconds);
}

//BloodDropletsを作成します。
void UEnemyHitFeedbackComponent::SpawnBloodDroplets(const FHitResult& _hitResult, const FVector& _shotDirection)
{
    //所有者を返します。
    AActor* owner = GetOwner();
    //ワールドを返します。
    UWorld* world = GetWorld();
    if (!IsValid(owner) || !world || !m_pDropletMesh || !m_pBloodDropletMaterial) { return; }
    FVector sprayDirection = (_hitResult.ImpactNormal - _shotDirection.GetSafeNormal() * 0.35f).GetSafeNormal();
    if (sprayDirection.IsNearlyZero())
    {
        sprayDirection = _hitResult.ImpactNormal;
    }

    //「int32 i = 0; i < m_dropletCount; ++i」で列挙される各要素へ、ループ本体の判定と更新を適用します。
    for (int32 i = 0; i < m_dropletCount; ++i)
    {
        UStaticMeshComponent* dropletComponent = NewObject<UStaticMeshComponent>(owner);
        if (!dropletComponent)
        {
            continue;
        }

        dropletComponent->SetStaticMesh(m_pDropletMesh);
        dropletComponent->SetMaterial(0, m_pBloodDropletMaterial);
        dropletComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        dropletComponent->SetCastShadow(false);
        dropletComponent->RegisterComponentWithWorld(world);
        dropletComponent->SetWorldLocation(_hitResult.ImpactPoint + _hitResult.ImpactNormal * BloodDropletSurfaceOffset);

        //現在の状態に合わせた移動倍率を用意します。
        const float scale = FMath::FRandRange(BloodDropletMinScale, BloodDropletMaxScale);
        dropletComponent->SetWorldScale3D(FVector(scale));
        FActiveBloodDroplet& droplet = m_activeBloodDroplets.AddDefaulted_GetRef();
        droplet.m_pComponent = dropletComponent;
        droplet.m_velocity = FMath::VRandCone(sprayDirection, FMath::DegreesToRadians(BloodDropletConeDegrees)) *
                             FMath::FRandRange(m_dropletSpeedMin, m_dropletSpeedMax);
        droplet.m_initialScale = FVector(scale);
        droplet.m_remainingLife = m_dropletLifeSeconds;
        droplet.m_totalLife = m_dropletLifeSeconds;
    }
}
