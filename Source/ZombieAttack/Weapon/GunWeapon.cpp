#include "GunWeapon.h"

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
    //リロード中、または弾切れの場合は銃側だけで勝手にReloadしません。
    //プレイヤー操作の場合は PlayerChara::Attack() が ReloadWeapon() を呼び、
    //リロードMontageと状態フラグを正しく動かします。
    if (m_bIsReloading || m_currentClipAmmo <= 0)
    {
        //「m_pEmptySound && m_currentClipAmmo <= 0 && m_currentTotalAmmo <= 0」が成立するとき、PlaySoundAtLocationを呼び出します。
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
    //「!IsValid(ownerController)」が成立するとき、続けて「!m_pWeapon」を判定します。
    if (!IsValid(ownerController)) { return; }

    if (!m_pWeapon)
    {
        return;
    }

    //cameraStartは、直後の初期化結果を、同じスコープ内でこの名前を参照する計算や関数呼び出しへ渡すために使います。
    FVector cameraStart;
    //cameraEndは、直後の初期化結果を、同じスコープ内でこの名前を参照する計算や関数呼び出しへ渡すために使います。
    FVector cameraEnd;
    //targetPointは、位置と向きの計算結果を移動、照準、または描画位置へ反映するために使います。
    FVector targetPoint;
    //aimHitResultは、衝突判定の結果を受け取り、命中位置や対象を参照するために使います。
    FHitResult aimHitResult;

    //カメラ中央からTraceします。
    //先にPawn専用Traceを見るため、近距離の敵もクロスヘア通りに拾いやすくなります。
    if (!GetCameraAimTarget(cameraStart, cameraEnd, targetPoint, aimHitResult)) { return; }

    //カメラ方向を保持します。
    const FVector cameraDirection = (cameraEnd - cameraStart).GetSafeNormal();

    //カメラレイで命中したActorにダメージを確定させます。
    //これにより、近距離で銃口位置のズレにより弾が外れる問題を避けます。
    const bool bCameraRayDamageApplied = m_bUseCameraRayDamage && aimHitResult.bBlockingHit && IsValid(aimHitResult.GetActor()) &&
                                         aimHitResult.GetActor() != ownerCharacter && aimHitResult.GetActor() != this;

    //「bCameraRayDamageApplied」が成立するとき、ApplyCameraRayDamageを呼び出します。
    if (bCameraRayDamageApplied)
    {
        ApplyCameraRayDamage(aimHitResult, cameraDirection, ownerController);
        SpawnCameraRayImpactEffect(aimHitResult);
    }

    //muzzleSocketNameは、TEXT("MuzzleFlashSocket")から構築した結果を後続の処理へ渡すために使います。
    const FName muzzleSocketName = TEXT("MuzzleFlashSocket");
    //muzzleLocationは、m_pWeapon->GetSocketLocation(muzzleSocketName)から求めた空間情報を位置または向きの計算に使います。
    FVector muzzleLocation = m_pWeapon->GetSocketLocation(muzzleSocketName);

    //finalDirectionは、(targetPoint - muzzleLocation).GetSafeNormal()から求めた空間情報を位置または向きの計算に使います。
    FVector finalDirection = (targetPoint - muzzleLocation).GetSafeNormal();
    //「finalDirection.IsNearlyZero()」が成立するとき、finalDirectionを更新します。
    if (finalDirection.IsNearlyZero())
    {
        finalDirection = cameraDirection;
    }

    muzzleLocation += finalDirection * 10.0f;
    //bulletRotationは、finalDirection.Rotation()の成立可否を後続の分岐で判定するために使います。
    const FRotator bulletRotation = finalDirection.Rotation();

    //spawnParamsは、Actor生成時の所有者や衝突時の生成規則を指定するために使います。
    FActorSpawnParameters spawnParams;
    spawnParams.Owner = this;
    spawnParams.Instigator = ownerCharacter;
    spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    //spawnedBulletは、GetWorld()->SpawnActor<ABullet>(m_bulletClass, muzzleLocation, bulletRo…から取得した参照を後続の呼び出しで使います。
    ABullet* spawnedBullet = GetWorld()->SpawnActor<ABullet>(m_bulletClass, muzzleLocation, bulletRotation, spawnParams);

    //「IsValid(spawnedBullet)」が成立するとき、SetMoveDirectionを呼び出します。
    if (IsValid(spawnedBullet))
    {
        spawnedBullet->SetMoveDirection(finalDirection);
        spawnedBullet->IgnoreActor(ownerCharacter);
        spawnedBullet->IgnoreActor(this);

        //「bCameraRayDamageApplied」が成立するとき、SetDamageEnabledを呼び出します。
        if (bCameraRayDamageApplied)
        {
            //ダメージはGunWeapon側でカメラレイにより確定済みです。
            //Bullet側では二重ダメージと二重ImpactEffectを防ぎます。
            spawnedBullet->SetDamageEnabled(false);
            spawnedBullet->SetImpactEffect(nullptr, m_impactEffectLifetime);
        }
        else
        {
            //何もカメラレイに当たらなかった場合は、従来通り弾ActorのHitで処理します。
            spawnedBullet->SetDamageEnabled(true);
            spawnedBullet->SetImpactEffect(m_impactEffect, m_impactEffectLifetime);
        }
    }

    --m_currentClipAmmo;
    OnReloadFinished.Broadcast();

    //「m_pFireSound」が成立するとき、PlaySoundAtLocationを呼び出します。
    if (m_pFireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, m_pFireSound, muzzleLocation);
    }

    //ワールドを返します。
    UAISense_Hearing::ReportNoiseEvent(GetWorld(), muzzleLocation, 1.0f, ownerCharacter, 0.0f, FName("Gunshot"));

    //弾の生成位置と向きをそのまま使い、武器ソケットの回転誤差を排除します。
    SpawnMuzzleFlash(muzzleLocation, bulletRotation);

}

//カメラの視線からターゲットポイントを取得する関数です。
bool AGunWeapon::GetCameraAimTarget(FVector& _outCameraStart, FVector& _outCameraEnd, FVector& _outTargetPoint, FHitResult& _outHitResult) const
{
    _outCameraStart = FVector::ZeroVector;
    _outCameraEnd = FVector::ZeroVector;
    _outTargetPoint = FVector::ZeroVector;
    _outHitResult = FHitResult();

    //ワールドを返します。
    UWorld* world = GetWorld();
    //「!world」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (!world) { return false; }

    //所有者キャラクターを保持します。
    ACharacter* ownerCharacter = Cast<ACharacter>(m_pOwnerChara);
    //「!IsValid(ownerCharacter)」が成立するとき、GetControllerを呼び出します。
    if (!IsValid(ownerCharacter)) { return false; }

    //コントローラーを返します。
    AController* ownerController = ownerCharacter->GetController();
    //「!IsValid(ownerController)」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (!IsValid(ownerController)) { return false; }

    //カメラ位置を保持します。
    FVector cameraLocation;
    //カメラ回転を保持します。
    FRotator cameraRotation;
    ownerController->GetPlayerViewPoint(cameraLocation, cameraRotation);

    //cameraForwardは、cameraRotation.Vector()から求めた空間情報を位置または向きの計算に使います。
    const FVector cameraForward = cameraRotation.Vector();
    _outCameraStart = cameraLocation;
    _outCameraEnd = cameraLocation + cameraForward * m_range;

    //queryParamsは、queryParamsの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    FCollisionQueryParams queryParams(SCENE_QUERY_STAT(PlayerAimTrace), true);
    queryParams.AddIgnoredActor(ownerCharacter);
    queryParams.AddIgnoredActor(this);
    queryParams.bTraceComplex = true;

    //まずPawnをObjectQueryで拾います。
    //敵のVisibility設定がIgnoreでも、Pawnとして存在していれば命中点を取りやすくします。
    if (m_pawnAimTraceSphereRadius > 0.0f)
    {
        //pawnObjectParamsは、トレースや関数呼び出しへ渡す検索条件を設定するために使います。
        FCollisionObjectQueryParams pawnObjectParams;
        pawnObjectParams.AddObjectTypesToQuery(ECC_Pawn);

        //pawnHitResultは、衝突判定の結果を受け取り、命中位置や対象を参照するために使います。
        FHitResult pawnHitResult;
        //射線がPawnへ命中したかを示します。
        const bool bPawnHit = world->SweepSingleByObjectType(pawnHitResult, _outCameraStart, _outCameraEnd, FQuat::Identity, pawnObjectParams,
                                                             FCollisionShape::MakeSphere(m_pawnAimTraceSphereRadius), queryParams);

        //「bPawnHit && IsValid(pawnHitResult.GetActor())」が成立するとき、_outHitResultを更新します。
        if (bPawnHit && IsValid(pawnHitResult.GetActor()))
        {
            _outHitResult = pawnHitResult;
            _outTargetPoint = pawnHitResult.ImpactPoint;
            //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
            return true;
        }
    }

    //Traceが衝突対象を検出したかを示します。
    bool bHit = false;
    //visibilityHitResultは、衝突判定の結果を受け取り、命中位置や対象を参照するために使います。
    FHitResult visibilityHitResult;

    //「m_aimTraceSphereRadius > 0.0f」が成立するとき、SweepSingleByChannelを呼び出します。
    if (m_aimTraceSphereRadius > 0.0f)
    {
        bHit = world->SweepSingleByChannel(visibilityHitResult, _outCameraStart, _outCameraEnd, FQuat::Identity, ECC_Visibility,
                                           FCollisionShape::MakeSphere(m_aimTraceSphereRadius), queryParams);
    }
    else
    {
        bHit = world->LineTraceSingleByChannel(visibilityHitResult, _outCameraStart, _outCameraEnd, ECC_Visibility, queryParams);
    }

    //「bHit」が成立するとき、_outHitResultを更新します。
    if (bHit)
    {
        _outHitResult = visibilityHitResult;
        _outTargetPoint = visibilityHitResult.ImpactPoint;
        //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
        return true;
    }

    _outTargetPoint = _outCameraEnd;
    //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
    return true;
}

//カメラRayダメージを対象へ適用します。
void AGunWeapon::ApplyCameraRayDamage(const FHitResult& _hitResult, const FVector& _damageDirection, AController* _ownerController)
{
    //hitActorは、_hitResult.GetActor()から取得した参照を後続の呼び出しで使います。
    AActor* hitActor = _hitResult.GetActor();
    //「!IsValid(hitActor)」が成立するとき、続けて「AEnemyChara* hitEnemy = Cast<AEnemyChara>(hitActor)」を判定します。
    if (!IsValid(hitActor)) { return; }

    //生物への命中表現は敵側の共通コンポーネントへ任せる
    if (AEnemyChara* hitEnemy = Cast<AEnemyChara>(hitActor))
    {
        hitEnemy->PlayBulletImpactFeedback(_hitResult, _damageDirection);
    }

    UGameplayStatics::ApplyPointDamage(hitActor, m_Damage, _damageDirection, _hitResult, _ownerController, this, UDamageType::StaticClass());
}

//カメラRayImpactエフェクトを作成します。
void AGunWeapon::SpawnCameraRayImpactEffect(const FHitResult& _hitResult)
{
    //敵へ命中した場合は血液演出が再生されるため、汎用Impactを重ねない
    if (Cast<AEnemyChara>(_hitResult.GetActor())) { return; }

    //「!m_impactEffect」が成立するとき、GetWorldを呼び出します。
    if (!m_impactEffect) { return; }

    //ワールドを返します。
    UWorld* world = GetWorld();
    //「!world」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
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
    //「!m_muzzleFlash」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
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
    //「!IsValid(_component)」が成立するとき、GetWorldを呼び出します。
    if (!IsValid(_component)) { return; }

    //ワールドを返します。
    UWorld* world = GetWorld();
    //「!world」が成立するとき、Deactivateを呼び出します。
    if (!world)
    {
        _component->Deactivate();
        _component->DestroyComponent();
        return;
    }

    //weakComponentは、_componentから取得した参照を後続の呼び出しで使います。
    TWeakObjectPtr<UNiagaraComponent> weakComponent = _component;
    //timerHandleは、タイマーの登録と解除を同じハンドルで管理するために使います。
    FTimerHandle timerHandle;

    world->GetTimerManager().SetTimer(timerHandle,
                                      FTimerDelegate::CreateLambda(
                                          [weakComponent]()
                                          {
                                              //「!weakComponent.IsValid()」が成立するとき、Getを呼び出します。
                                              if (!weakComponent.IsValid()) { return; }

                                              //componentは、weakComponent.Get()から取得した参照を後続の呼び出しで使います。
                                              UNiagaraComponent* component = weakComponent.Get();
                                              //「!IsValid(component)」が成立するとき、Deactivateを呼び出します。
                                              if (!IsValid(component)) { return; }

                                              component->Deactivate();
                                              component->DestroyComponent();
                                          }),
                                      FMath::Max(0.01f, _delaySeconds), false);
}

//リロード処理です。
void AGunWeapon::Reload()
{
    //「m_bIsReloading || m_currentClipAmmo == m_maxClipAmmo || m_currentTotalAmmo <= 0」が成立するとき、m_bIsReloadingを更新します。
    if (m_bIsReloading || m_currentClipAmmo == m_maxClipAmmo || m_currentTotalAmmo <= 0) { return; }

    m_bIsReloading = true;
}

//リロード完了処理です。
void AGunWeapon::FinishReload()
{
    //「!m_bIsReloading」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (!m_bIsReloading) { return; }

    //neededAmmoは、m_maxClipAmmo - m_currentClipAmmoから算出した数値を後続の判定または計算に使います。
    const int32 neededAmmo = m_maxClipAmmo - m_currentClipAmmo;
    //ammoToReloadは、FMath::Min(neededAmmo, m_currentTotalAmmo)から算出した数値を後続の判定または計算に使います。
    const int32 ammoToReload = FMath::Min(neededAmmo, m_currentTotalAmmo);

    m_currentClipAmmo += ammoToReload;
    m_currentTotalAmmo -= ammoToReload;
    m_bIsReloading = false;

    OnReloadFinished.Broadcast();
}

//予備弾を追加します。
void AGunWeapon::AddAmmo(float _amount)
{
    //amountToAddは、FMath::RoundToInt(_amount)から算出した数値を後続の判定または計算に使います。
    const int32 amountToAdd = FMath::RoundToInt(_amount);
    m_currentTotalAmmo = FMath::Clamp(m_currentTotalAmmo + amountToAdd, 0, m_totalMaxAmmo);
    OnReloadFinished.Broadcast();
}

//リロード可能かどうかを返します。
bool AGunWeapon::CanReload() const
{
    //「m_currentClipAmmo >= m_maxClipAmmo」が成立するとき、続けて「m_currentTotalAmmo <= 0」を判定します。
    if (m_currentClipAmmo >= m_maxClipAmmo) { return false; }

    //「m_currentTotalAmmo <= 0」が成立するとき、この関数を終了します。
    if (m_currentTotalAmmo <= 0) { return false; }

    //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
    return true;
}
