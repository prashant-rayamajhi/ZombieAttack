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
constexpr float BloodDropletMinScale = 0.030f;
constexpr float BloodDropletMaxScale = 0.058f;
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
    //「bloodDecalMaterial.Succeeded()」が成立するとき、m_pBloodDecalMaterialを更新します。
    if (bloodDecalMaterial.Succeeded())
    {
        m_pBloodDecalMaterial = bloodDecalMaterial.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> bloodDropletMaterial(
        TEXT("/Game/VFX/Blood/Materials/M_BloodDroplet.M_BloodDroplet"));
    //「bloodDropletMaterial.Succeeded()」が成立するとき、m_pBloodDropletMaterialを更新します。
    if (bloodDropletMaterial.Succeeded())
    {
        m_pBloodDropletMaterial = bloodDropletMaterial.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> bloodImpactSpriteMaterial(
        TEXT("/Game/VFX/Blood/Materials/M_BloodImpactSprite.M_BloodImpactSprite"));
    //「bloodImpactSpriteMaterial.Succeeded()」が成立するとき、m_pBloodImpactSpriteMaterialを更新します。
    if (bloodImpactSpriteMaterial.Succeeded())
    {
        m_pBloodImpactSpriteMaterial = bloodImpactSpriteMaterial.Object;
    }

    //dropletMeshは、dropletMeshの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    static ConstructorHelpers::FObjectFinder<UStaticMesh> dropletMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    //「dropletMesh.Succeeded()」が成立するとき、m_pDropletMeshを更新します。
    if (dropletMesh.Succeeded())
    {
        m_pDropletMesh = dropletMesh.Object;
    }

    //impactPlaneMeshは、impactPlaneMeshの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    static ConstructorHelpers::FObjectFinder<UStaticMesh> impactPlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
    //「impactPlaneMesh.Succeeded()」が成立するとき、m_pImpactPlaneMeshを更新します。
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
        //dropletは、m_activeBloodDroplets[i]から構築した結果を後続の処理へ渡すために使います。
        FActiveBloodDroplet& droplet = m_activeBloodDroplets[i];
        //dropletComponentは、droplet.m_pComponent.Get()から取得した参照を後続の呼び出しで使います。
        UStaticMeshComponent* dropletComponent = droplet.m_pComponent.Get();
        //「!IsValid(dropletComponent)」が成立するとき、RemoveAtSwapを呼び出します。
        if (!IsValid(dropletComponent))
        {
            m_activeBloodDroplets.RemoveAtSwap(i);
            continue;
        }

        droplet.m_remainingLife -= _deltaTime;
        //「droplet.m_remainingLife <= 0.0f」が成立するとき、DestroyComponentを呼び出します。
        if (droplet.m_remainingLife <= 0.0f)
        {
            dropletComponent->DestroyComponent();
            m_activeBloodDroplets.RemoveAtSwap(i);
            continue;
        }

        droplet.m_velocity.Z -= m_dropletGravity * _deltaTime;
        dropletComponent->AddWorldOffset(droplet.m_velocity * _deltaTime, false);

        //lifeAlphaは、FMath::Clamp(droplet.m_remainingLife / droplet.m_totalLife, 0.0f, 1.0f)から算出した数値を後続の判定または計算に使います。
        const float lifeAlpha = FMath::Clamp(droplet.m_remainingLife / droplet.m_totalLife, 0.0f, 1.0f);
        dropletComponent->SetWorldScale3D(droplet.m_initialScale * lifeAlpha);
    }

    //「int32 i = m_activeBloodSprites.Num() - 1; i >= 0; --i」で列挙される各要素へ、ループ本体の判定と更新を適用します。
    for (int32 i = m_activeBloodSprites.Num() - 1; i >= 0; --i)
    {
        //spriteは、m_activeBloodSprites[i]から構築した結果を後続の処理へ渡すために使います。
        FActiveBloodSprite& sprite = m_activeBloodSprites[i];
        //spriteComponentは、sprite.m_pComponent.Get()から取得した参照を後続の呼び出しで使います。
        UStaticMeshComponent* spriteComponent = sprite.m_pComponent.Get();
        //「!IsValid(spriteComponent)」が成立するとき、RemoveAtSwapを呼び出します。
        if (!IsValid(spriteComponent))
        {
            m_activeBloodSprites.RemoveAtSwap(i);
            continue;
        }

        sprite.m_remainingLife -= _deltaTime;
        //「sprite.m_remainingLife <= 0.0f」が成立するとき、DestroyComponentを呼び出します。
        if (sprite.m_remainingLife <= 0.0f)
        {
            spriteComponent->DestroyComponent();
            m_activeBloodSprites.RemoveAtSwap(i);
            continue;
        }

        //elapsedAlphaは、FMath::Clamp(1.0f - sprite.m_remainingLife / sprite.m_totalLife, 0.0f, …から算出した数値を後続の判定または計算に使います。
        const float elapsedAlpha = FMath::Clamp(1.0f - sprite.m_remainingLife / sprite.m_totalLife, 0.0f, 1.0f);
        spriteComponent->SetWorldScale3D(sprite.m_initialScale *
                                         FMath::Lerp(BloodSpriteStartScaleMultiplier, BloodSpriteEndScaleMultiplier, elapsedAlpha));
    }
}

//BulletImpactを再生します。
void UEnemyHitFeedbackComponent::PlayBulletImpact(const FHitResult& _hitResult, const FVector& _shotDirection)
{
    //「!_hitResult.bBlockingHit」が成立するとき、SpawnBloodDecalを呼び出します。
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
    //「!IsValid(owner) || !world || !m_pImpactPlaneMesh || !m_pBloodImpactSpriteMaterial」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (!IsValid(owner) || !world || !m_pImpactPlaneMesh || !m_pBloodImpactSpriteMaterial) { return; }

    //spriteComponentは、NewObject<UStaticMeshComponent>(owner)から取得した参照を後続の呼び出しで使います。
    UStaticMeshComponent* spriteComponent = NewObject<UStaticMeshComponent>(owner);
    //「!spriteComponent」が成立するとき、GetSafeNormalを呼び出します。
    if (!spriteComponent) { return; }

    //facingDirectionは、-_shotDirection.GetSafeNormal()から求めた空間情報を位置または向きの計算に使います。
    FVector facingDirection = -_shotDirection.GetSafeNormal();
    //「facingDirection.IsNearlyZero()」が成立するとき、facingDirectionを更新します。
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
    spriteComponent->SetWorldScale3D(FVector(m_impactSpriteScale));

    //spriteは、m_activeBloodSprites.AddDefaulted_GetRef()から構築した結果を後続の処理へ渡すために使います。
    FActiveBloodSprite& sprite = m_activeBloodSprites.AddDefaulted_GetRef();
    sprite.m_pComponent = spriteComponent;
    sprite.m_initialScale = FVector(m_impactSpriteScale);
    sprite.m_remainingLife = m_impactSpriteLifeSeconds;
    sprite.m_totalLife = m_impactSpriteLifeSeconds;
}

//BloodDecalを作成します。
void UEnemyHitFeedbackComponent::SpawnBloodDecal(const FHitResult& _hitResult) const
{
    //コンポーネントを返します。
    UPrimitiveComponent* hitComponent = _hitResult.GetComponent();
    //「!m_pBloodDecalMaterial || !IsValid(hitComponent)」が成立するとき、Rotationを呼び出します。
    if (!m_pBloodDecalMaterial || !IsValid(hitComponent)) { return; }

    //decalRotationは、_hitResult.ImpactNormal.Rotation()から求めた空間情報を位置または向きの計算に使います。
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
    //「!IsValid(owner) || !world || !m_pDropletMesh || !m_pBloodDropletMaterial」が成立するとき、GetSafeNormalを呼び出します。
    if (!IsValid(owner) || !world || !m_pDropletMesh || !m_pBloodDropletMaterial) { return; }

    //sprayDirectionは、(_hitResult.ImpactNormal - _shotDirection.GetSafeNormal() * 0.35f).GetS…から求めた空間情報を位置または向きの計算に使います。
    FVector sprayDirection = (_hitResult.ImpactNormal - _shotDirection.GetSafeNormal() * 0.35f).GetSafeNormal();
    //「sprayDirection.IsNearlyZero()」が成立するとき、sprayDirectionを更新します。
    if (sprayDirection.IsNearlyZero())
    {
        sprayDirection = _hitResult.ImpactNormal;
    }

    //「int32 i = 0; i < m_dropletCount; ++i」で列挙される各要素へ、ループ本体の判定と更新を適用します。
    for (int32 i = 0; i < m_dropletCount; ++i)
    {
        //dropletComponentは、NewObject<UStaticMeshComponent>(owner)から取得した参照を後続の呼び出しで使います。
        UStaticMeshComponent* dropletComponent = NewObject<UStaticMeshComponent>(owner);
        //「!dropletComponent」が成立するとき、SetStaticMeshを呼び出します。
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

        //dropletは、m_activeBloodDroplets.AddDefaulted_GetRef()から構築した結果を後続の処理へ渡すために使います。
        FActiveBloodDroplet& droplet = m_activeBloodDroplets.AddDefaulted_GetRef();
        droplet.m_pComponent = dropletComponent;
        droplet.m_velocity = FMath::VRandCone(sprayDirection, FMath::DegreesToRadians(BloodDropletConeDegrees)) *
                             FMath::FRandRange(m_dropletSpeedMin, m_dropletSpeedMax);
        droplet.m_initialScale = FVector(scale);
        droplet.m_remainingLife = m_dropletLifeSeconds;
        droplet.m_totalLife = m_dropletLifeSeconds;
    }
}
