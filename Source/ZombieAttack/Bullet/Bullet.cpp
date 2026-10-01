#include "Bullet.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "../Enemy/EnemyChara.h"
#include "../Player/PlayerChara.h"

//ABulletが使用するComponentと初期パラメータを設定します。
ABullet::ABullet()
    : m_pProjectileComp(nullptr), m_pBulletMesh(nullptr), m_damage(20.0f), m_bDamageEnabled(true), m_moveDirection(FVector::ForwardVector),
      m_bHasHit(false), m_impactEffect(nullptr), m_impactEffectLifetime(0.75f)
{
    PrimaryActorTick.bCanEverTick = true;

    m_pBulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
    if (m_pBulletMesh)
    {
        //ルートコンポーネントに設定。
        RootComponent = m_pBulletMesh;

        //OnHitを使うため、敵PawnをOverlapではなくBlock
        m_pBulletMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        m_pBulletMesh->SetCollisionObjectType(ECC_WorldDynamic);
        m_pBulletMesh->SetCollisionResponseToAllChannels(ECR_Block);
        m_pBulletMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        m_pBulletMesh->SetNotifyRigidBodyCollision(true);
        m_pBulletMesh->OnComponentHit.AddDynamic(this, &ABullet::OnHit);
    }

    //弾の移動コンポーネントを作成
    m_pProjectileComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
    if (m_pProjectileComp)
    {
        m_pProjectileComp->InitialSpeed = 10000.0f;
        m_pProjectileComp->MaxSpeed = 20000.0f;
        m_pProjectileComp->ProjectileGravityScale = 0.0f;
        m_pProjectileComp->bRotationFollowsVelocity = true;
        m_pProjectileComp->bShouldBounce = false;
    }

    //弾の寿命を設定
    InitialLifeSpan = 3.0f;
}

//毎フレーム呼ばれる関数
void ABullet::Tick(float _deltaTime)
{
    //フレームごとの経過時間を使って、移動や表示の変化を更新します。
    Super::Tick(_deltaTime);
}

//銃側から渡された方向へ弾を飛ばす
void ABullet::SetMoveDirection(const FVector& _moveDirection)
{
    //方向ベクトルがゼロベクトルの場合は処理しない
    if (_moveDirection.IsNearlyZero()) { return; }

    //弾の移動方向を正規化して設定
    m_moveDirection = _moveDirection.GetSafeNormal();
    SetActorRotation(m_moveDirection.Rotation());

    //弾の移動コンポーネントの速度を設定
    if (m_pProjectileComp)
    {
        m_pProjectileComp->Velocity = m_moveDirection * m_pProjectileComp->InitialSpeed;
    }
}

//プレイヤーや武器へ弾が当たらないようにする
void ABullet::IgnoreActor(AActor* _actor)
{
    //引数が有効でない場合や、弾のメッシュが存在しない場合は処理を中止
    if (!IsValid(_actor) || !m_pBulletMesh) { return; }

    //弾のメッシュが移動する際に、指定されたアクターを無視するように設定
    m_pBulletMesh->IgnoreActorWhenMoving(_actor, true);
}

//銃側から命中エフェクトを受け取る
void ABullet::SetImpactEffect(UNiagaraSystem* _impactEffect, float _impactEffectLifetime)
{
    m_impactEffect = _impactEffect;
    m_impactEffectLifetime = FMath::Max(0.01f, _impactEffectLifetime);
}

//ダメージを与えるかどうかを設定する
void ABullet::SetDamageEnabled(bool _bEnabled) { m_bDamageEnabled = _bEnabled; }

//弾が何かに当たったときの処理
void ABullet::OnHit(UPrimitiveComponent* _hitComp, AActor* _otherActor, UPrimitiveComponent* _otherComp, FVector _normalImpulse,
                    const FHitResult& _hit)
{
    //すでに当たった場合は処理を中止
    if (m_bHasHit) { return; }

    //引数が有効でない場合や、弾自身に当たった場合は処理を中止
    if (!IsValid(_otherActor) || _otherActor == this) { return; }

    //武器本体、撃ったプレイヤー、弾のOwnerには当てません。
    if (_otherActor == GetOwner() || _otherActor == GetInstigator()) { return; }

    //当たったことを記録するフラグを立てる
    m_bHasHit = true;

    //敵は血液演出、壁や地面は武器共通のImpact演出を使用する
    if (!Cast<AEnemyChara>(_otherActor))
    {
        SpawnImpactEffectOnce(_hit);
    }

    //弾が当たったオブジェクトに物理的な衝撃を与える
    if (_otherComp && _otherComp->IsSimulatingPhysics() && !_otherActor->IsA<APlayerChara>())
    {
        const FVector impulseDirection = m_moveDirection.IsNearlyZero() ? GetActorForwardVector() : m_moveDirection;
        _otherComp->AddImpulseAtLocation(impulseDirection * 100000.0f, _hit.ImpactPoint);
    }

    //ダメージが有効でない場合は弾を破壊して処理を終了
    if (!m_bDamageEnabled)
    {
        Destroy();
        return;
    }

    //弾が当たった敵へ、ダメージより先に命中位置を渡す
    const FVector damageDirection = m_moveDirection.IsNearlyZero() ? GetActorForwardVector() : m_moveDirection;
    if (AEnemyChara* hitEnemy = Cast<AEnemyChara>(_otherActor))
    {
        hitEnemy->PlayBulletImpactFeedback(_hit, damageDirection);
    }

    //弾が当たったオブジェクトにダメージを与える
    UGameplayStatics::ApplyPointDamage(_otherActor, m_damage, damageDirection, _hit, GetInstigatorController(), this, nullptr);
    Destroy();
}

//命中エフェクトを一度だけ出す
void ABullet::SpawnImpactEffectOnce(const FHitResult& _hit)
{
    //命中エフェクトが設定されていない場合は処理を中止
    if (!m_impactEffect) { return; }

    //ワールドを取得
    UWorld* world = GetWorld();
    if (!world) { return; }

    //弾が当たった位置に命中エフェクトを生成
    UNiagaraComponent* impactComponent =
        //SystemAt位置を作成します。
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(world, m_impactEffect, _hit.ImpactPoint, _hit.ImpactNormal.Rotation(), FVector(1.0f), true,
                                                       true, ENCPoolMethod::None, true);

    //命中エフェクトを一定時間後に破棄する
    DestroyNiagaraAfterDelay(impactComponent, m_impactEffectLifetime);
}

//命中エフェクトを一定時間後に破棄する
void ABullet::DestroyNiagaraAfterDelay(UNiagaraComponent* _component, float _delaySeconds) const
{
    //引数が有効でない場合は処理を中止
    if (!IsValid(_component)) { return; }

    //ワールドを取得
    UWorld* world = GetWorld();
    if (!world)
    {
        _component->Deactivate();
        _component->DestroyComponent();
        return;
    }

    //破棄するNiagaraコンポーネントを弱参照で保持
    TWeakObjectPtr<UNiagaraComponent> weakComponent = _component;
    FTimerHandle timerHandle;

    //指定された遅延時間後にNiagaraコンポーネントを停止して破棄するタイマーを設定
    world->GetTimerManager().SetTimer(timerHandle,
                                      FTimerDelegate::CreateLambda(
                                          [weakComponent]()
                                          {
                                              if (!weakComponent.IsValid()) { return; }
                                              UNiagaraComponent* component = weakComponent.Get();
                                              if (!IsValid(component)) { return; }

                                              component->Deactivate();
                                              component->DestroyComponent();
                                          }),
                                      FMath::Max(0.01f, _delaySeconds), false);
}
