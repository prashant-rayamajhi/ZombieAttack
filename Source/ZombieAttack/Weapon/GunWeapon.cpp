#include "GunWeapon.h"
#include "WeaponAimTrace.h"

#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"
#include "../Bullet/Bullet.h"
#include "../Enemy/EnemyChara.h"
#include "../Player/PlayerChara.h"

//銃武器を処理します。
AGunWeapon::AGunWeapon()
    : m_range(8000.0f), m_fireRate(0.4f), m_aimTraceSphereRadius(6.0f), m_pawnAimTraceSphereRadius(22.0f), m_bUseCameraRayDamage(true),
      m_muzzleFlash(nullptr), m_impactEffect(nullptr), m_muzzleFlashLifetime(0.08f), m_muzzleFlashScale(0.18f),
      m_impactEffectLifetime(0.75f), m_bulletClass(nullptr), m_maxClipAmmo(6), m_totalMaxAmmo(200), m_currentClipAmmo(6), m_currentTotalAmmo(200),
      m_bIsReloading(false), m_pFireSound(nullptr), m_pEmptySound(nullptr), m_pReloadSound(nullptr)
{
}

//弾を発射する関数です。
void AGunWeapon::UseWeapon()
{
    if (!CanFireNow()) { return; }
    //リロード中、または弾切れの場合は銃側だけで勝手にReloadしません。
    //プレイヤー操作の場合は PlayerChara::Attack() が ReloadWeapon() を呼び、
    //リロードMontageと状態フラグを正しく動かします。
    if (m_bIsReloading || m_currentClipAmmo <= 0)
    {
        if (m_pEmptySound && m_currentClipAmmo <= 0 && m_currentTotalAmmo <= 0)
        {
            UGameplayStatics::PlaySoundAtLocation(this, m_pEmptySound, GetActorLocation());
        }
        return;
    }

    if (!m_bulletClass)
    {
        return;
    }

    //所有者キャラクターを保持します。
    ACharacter* ownerCharacter = Cast<ACharacter>(m_pOwnerChara);
    if (!IsValid(ownerCharacter))
    {
        return;
    }

    //コントローラーを返します。
    AController* ownerController = ownerCharacter->GetController();
    if (!IsValid(ownerController)) { return; }

    if (!m_pWeapon)
    {
        return;
    }
    FVector cameraStart;
    FVector cameraEnd;
    FVector targetPoint;
    FHitResult aimHitResult;

    //画面中央の最も手前の物体を、銃口が狙う位置として取得する。
    if (!GetCameraAimTarget(cameraStart, cameraEnd, targetPoint, aimHitResult)) { return; }

    //カメラ方向を保持します。
    const FVector cameraDirection = (cameraEnd - cameraStart).GetSafeNormal();

    //画面中央は狙いを決めるだけに使い、実際の命中は銃口から通る射線で決める。
    const FVector muzzleLocation = m_pWeapon->GetSocketLocation(TEXT("MuzzleFlashSocket"));
    //照準点が銃口より後ろにある場合、弾を自分の方へ折り返さない。
    if (FVector::DotProduct(targetPoint - muzzleLocation, cameraDirection) <= 0.0f)
    {
        targetPoint = muzzleLocation + cameraDirection * m_range;
    }
    FVector finalDirection = (targetPoint - muzzleLocation).GetSafeNormal();
    if (finalDirection.IsNearlyZero()) { finalDirection = cameraDirection; }
    FHitResult shotHit;
    //銃身が壁へめり込んだ場合も、体から銃口までの遮蔽物で発砲を止める。
    const FVector shoulder = ownerCharacter->GetActorLocation() + FVector(0, 0, 40);
    const bool barrelBlocked = WeaponAimTrace::FindFirstHit(GetWorld(), shoulder, muzzleLocation, ownerCharacter, this, shotHit);
    bool blocked = barrelBlocked;
    if (!blocked)
    {
        blocked = WeaponAimTrace::FindFirstHit(GetWorld(), muzzleLocation, targetPoint + finalDirection * 2.0f,
            ownerCharacter, this, shotHit);
    }
    const bool rayDamage = (m_bUseCameraRayDamage || barrelBlocked) && blocked;
    if (rayDamage)
    {
        ApplyShotDamage(shotHit, finalDirection, ownerController);
        SpawnShotImpactEffect(shotHit);
    }
    const FRotator bulletRotation = finalDirection.Rotation();
    FActorSpawnParameters spawnParams;
    spawnParams.Owner = this;
    spawnParams.Instigator = ownerCharacter;
    spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    //銃口が壁の向こうへ突き抜けたときは、その先に見た目の弾も生成しない。
    ABullet* spawnedBullet = barrelBlocked ? nullptr : GetWorld()->SpawnActor<ABullet>(m_bulletClass, muzzleLocation, bulletRotation, spawnParams);
    if (IsValid(spawnedBullet))
    {
        spawnedBullet->SetMoveDirection(finalDirection);
        spawnedBullet->IgnoreActor(ownerCharacter);
        spawnedBullet->IgnoreActor(this);
        if (m_bUseCameraRayDamage || barrelBlocked)
        {
            //銃口からの射線で判定済みなので、見た目の弾から二重にダメージを与えない。
            spawnedBullet->SetDamageEnabled(false);
            //即時命中に対して弾だけが遅れて見えないよう、曳光の速度を実弾に近づける。
            auto* movement = spawnedBullet->FindComponentByClass<UProjectileMovementComponent>();
            if (movement)
            {
                movement->InitialSpeed = FMath::Max(60000.0f, movement->InitialSpeed);
                movement->MaxSpeed = FMath::Max(movement->MaxSpeed, movement->InitialSpeed);
                spawnedBullet->SetMoveDirection(finalDirection);
            }
            if (blocked)
            {
                const float speed = movement ? movement->InitialSpeed : 60000.0f;
                spawnedBullet->SetLifeSpan(FMath::Max(0.01f, FVector::Distance(muzzleLocation, shotHit.ImpactPoint) / FMath::Max(speed, 1.0f)));
            }
            spawnedBullet->SetImpactEffect(nullptr, m_impactEffectLifetime);
        }
        else
        {
            //実体弾を選んだ武器では、飛翔後の衝突にダメージ判定を任せる。
            spawnedBullet->SetDamageEnabled(true);
            spawnedBullet->SetImpactEffect(m_impactEffect, m_impactEffectLifetime);
        }
    }

    --m_currentClipAmmo;
    m_nextShotTime = GetWorld()->GetTimeSeconds() + GetFireRate();
    m_onReloadFinished.Broadcast();
    if (m_pFireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, m_pFireSound, muzzleLocation);
    }

    //ワールドを返します。
    UAISense_Hearing::ReportNoiseEvent(GetWorld(), muzzleLocation, 1.0f, ownerCharacter, 0.0f, FName("Gunshot"));

    //弾の生成位置と向きをそのまま使い、武器ソケットの回転誤差を排除します。
    SpawnMuzzleFlash(muzzleLocation, bulletRotation);

}

//リロード演出や入力回数に関係なく、武器で決めた発射間隔を守る。
bool AGunWeapon::CanFireNow() const
{
    return GetWorld() && GetWorld()->GetTimeSeconds() + 0.001 >= m_nextShotTime;
}

//画面中央で最初に遮られる点を求め、銃口から狙う位置として使う。
bool AGunWeapon::GetCameraAimTarget(FVector& _outCameraStart, FVector& _outCameraEnd, FVector& _outTargetPoint, FHitResult& _outHitResult) const
{
    _outCameraStart = FVector::ZeroVector;
    _outCameraEnd = FVector::ZeroVector;
    _outTargetPoint = FVector::ZeroVector;
    _outHitResult = FHitResult();

    //ワールドを返します。
    UWorld* world = GetWorld();
    if (!world) { return false; }

    //所有者キャラクターを保持します。
    ACharacter* ownerCharacter = Cast<ACharacter>(m_pOwnerChara);
    if (!IsValid(ownerCharacter)) { return false; }

    //コントローラーを返します。
    AController* ownerController = ownerCharacter->GetController();
    if (!IsValid(ownerController)) { return false; }

    //カメラ位置を保持します。
    FVector cameraLocation;
    //カメラ回転を保持します。
    FRotator cameraRotation;
    ownerController->GetPlayerViewPoint(cameraLocation, cameraRotation);
    const FVector cameraForward = cameraRotation.Vector();
    _outCameraStart = cameraLocation;
    _outCameraEnd = cameraLocation + cameraForward * m_range;

    //照準補助の太い球で敵を優先せず、中央の細い射線に最初に当たる場所を狙う。
    if (WeaponAimTrace::FindFirstHit(world, _outCameraStart, _outCameraEnd, ownerCharacter, const_cast<AGunWeapon*>(this), _outHitResult))
    {
        _outTargetPoint = _outHitResult.ImpactPoint;
        return true;
    }

    _outTargetPoint = _outCameraEnd;
    //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
    return true;
}

//銃口から最初に当たった対象へ命中演出とダメージを一度だけ渡す。
void AGunWeapon::ApplyShotDamage(const FHitResult& _hitResult, const FVector& _damageDirection, AController* _ownerController)
{
    AActor* hitActor = _hitResult.GetActor();
    if (!IsValid(hitActor)) { return; }

    //生物への命中表現は敵側の共通コンポーネントへ任せる
    if (AEnemyChara* hitEnemy = Cast<AEnemyChara>(hitActor))
    {
        hitEnemy->PlayBulletImpactFeedback(_hitResult, _damageDirection);
    }

    UGameplayStatics::ApplyPointDamage(hitActor, m_damage, _damageDirection, _hitResult, _ownerController, this, UDamageType::StaticClass());
}

//壁や地面へ着弾した場所に、武器ごとの命中エフェクトを出す。
void AGunWeapon::SpawnShotImpactEffect(const FHitResult& _hitResult)
{
    //敵へ命中した場合は血液演出が再生されるため、汎用Impactを重ねない
    if (Cast<AEnemyChara>(_hitResult.GetActor())) { return; }
    if (!m_impactEffect) { return; }

    //ワールドを返します。
    UWorld* world = GetWorld();
    if (!world) { return; }

    UNiagaraComponent* impactComponent =
        //SystemAt位置を作成します。
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(world, m_impactEffect, _hitResult.ImpactPoint, _hitResult.ImpactNormal.Rotation(),
                                                       FVector(1.0f), true, true, ENCPoolMethod::None, true);

    DestroyNiagaraAfterDelay(impactComponent, m_impactEffectLifetime);
}

//MuzzleFlashを生成する関数です。
void AGunWeapon::SpawnMuzzleFlash(const FVector& _location, const FRotator& _rotation)
{
    if (!m_muzzleFlash) { return; }

    UNiagaraComponent* flashComponent =
        //ワールドを返します。
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), m_muzzleFlash, _location, _rotation, FVector(m_muzzleFlashScale), true, true,
                                                       ENCPoolMethod::None, true);

    DestroyNiagaraAfterDelay(flashComponent, m_muzzleFlashLifetime);

}

//NiagaraAfterDelayを解除します。
void AGunWeapon::DestroyNiagaraAfterDelay(UNiagaraComponent* _component, float _delaySeconds) const
{
    if (!IsValid(_component)) { return; }

    //ワールドを返します。
    UWorld* world = GetWorld();
    if (!world)
    {
        _component->Deactivate();
        _component->DestroyComponent();
        return;
    }
    TWeakObjectPtr<UNiagaraComponent> weakComponent = _component;
    FTimerHandle timerHandle;

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

//リロード処理です。
void AGunWeapon::Reload()
{
    if (m_bIsReloading || m_currentClipAmmo == m_maxClipAmmo || m_currentTotalAmmo <= 0) { return; }

    m_bIsReloading = true;
}

//リロード完了処理です。
void AGunWeapon::FinishReload()
{
    if (!m_bIsReloading) { return; }
    const int32 neededAmmo = m_maxClipAmmo - m_currentClipAmmo;
    const int32 ammoToReload = FMath::Min(neededAmmo, m_currentTotalAmmo);

    m_currentClipAmmo += ammoToReload;
    m_currentTotalAmmo -= ammoToReload;
    m_bIsReloading = false;

    m_onReloadFinished.Broadcast();
}

//予備弾を追加します。
void AGunWeapon::AddAmmo(float _amount)
{
    const int32 amountToAdd = FMath::RoundToInt(_amount);
    m_currentTotalAmmo = FMath::Clamp(m_currentTotalAmmo + amountToAdd, 0, m_totalMaxAmmo);
    m_onReloadFinished.Broadcast();
}

//リロード可能かどうかを返します。
bool AGunWeapon::CanReload() const
{
    if (m_currentClipAmmo >= m_maxClipAmmo) { return false; }
    if (m_currentTotalAmmo <= 0) { return false; }

    //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
    return true;
}
