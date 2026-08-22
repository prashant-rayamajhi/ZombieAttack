#include "PlayerChara.h"
#include "../Components/PlayerAudio/PlayerAudioComponent.h"
#include "../Weapon/ARWeapon.h"
#include "../Weapon/GunWeapon.h"
#include "../Weapon/WeaponBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

//受けたダメージを体力へ反映します。
float APlayerChara::TakeDamage(float _damageAmount, FDamageEvent const& _damageEvent, AController* _eventInstigator, AActor* _damageCauser)
{
    //「m_bIsDead || _damageAmount <= 0.f」が成立するとき、TakeDamageを呼び出します。
    if (m_bIsDead || _damageAmount <= 0.f) { return 0.f; }

    //ダメージを保持します。
    const float damage = Super::TakeDamage(_damageAmount, _damageEvent, _eventInstigator, _damageCauser);
    //「damage <= 0.f」が成立するとき、Clampを呼び出します。
    if (damage <= 0.f) { return 0.f; }

    m_Hp = FMath::Clamp(m_Hp - damage, 0.f, m_MaxHp);
    OnDamaged.Broadcast();

    //「m_Hp <= 0.f」が成立するとき、BeginDeathSequenceを呼び出します。
    if (m_Hp <= 0.f)
    {
        BeginDeathSequence();
    }
    //damageは、ゲーム判定に使用する数値を計算し、後続の比較または更新へ渡すために使います。
    return damage;
}

//KillHitStopを再生します。
void APlayerChara::PlayKillHitStop()
{
    //ワールドを返します。
    UWorld* world = GetWorld();
    //「!world || m_bIsDead」が成立するとき、GetWorldTimerManagerを呼び出します。
    if (!world || m_bIsDead) { return; }

    //連続撃破では終了タイマーを更新し、途中で通常速度へ戻るちらつきを防ぎます。
    GetWorldTimerManager().ClearTimer(m_killHitStopTimer);
    UGameplayStatics::SetGlobalTimeDilation(world, m_killHitStopTimeDilation);

    //WorldTimerはゲーム時間で進むため、実時間の長さになるようDilationを掛けます。
    const float timerDuration = m_killHitStopDuration * FMath::Max(m_killHitStopTimeDilation, 0.01f);
    GetWorldTimerManager().SetTimer(m_killHitStopTimer, this, &APlayerChara::RestoreKillHitStop, timerDuration, false);
}

//KillHitStopを変更前の状態へ戻します。
void APlayerChara::RestoreKillHitStop()
{
    //「UWorld* world = GetWorld()」が成立するとき、SetGlobalTimeDilationを呼び出します。
    if (UWorld* world = GetWorld())
    {
        UGameplayStatics::SetGlobalTimeDilation(world, 1.0f);
    }
}

//死亡Sequenceを開始するための状態を設定します。
void APlayerChara::BeginDeathSequence()
{
    //「m_bIsDead」が成立するとき、m_bIsDeadを更新します。
    if (m_bIsDead) { return; }

    m_bIsDead = true;
    m_bCanControl = false;
    m_charaMovement = FVector2D::ZeroVector;
    m_bIsAiming = false;
    m_bIsFiringRifle = false;
    m_bRifleCombatAim = false;
    m_bPendingRifleShot = false;
    StopRifleAimPose(0.0f);
    m_bIsHealing = false;

    //「AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon)」が成立するとき、CancelReloadを呼び出します。
    if (AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon))
    {
        gunWeapon->CancelReload();
    }
    //「m_pAudioComponent」が成立するとき、StopReloadSoundを呼び出します。
    if (m_pAudioComponent)
    {
        m_pAudioComponent->StopReloadSound();
    }

    m_bIsReloadingAnim = false;
    m_bIsSwitchingWeapon = false;
    m_bMovementLockedByAnimation = false;

    BroadcastDeathOnce();
    GetWorldTimerManager().ClearTimer(m_switchWeaponTimer);
    GetWorldTimerManager().ClearTimer(m_healFallbackTimer);
    GetWorldTimerManager().ClearTimer(m_movementLockTimer);

    //「AController* playerController = GetController()」が成立するとき、StopMovementを呼び出します。
    if (AController* playerController = GetController())
    {
        playerController->StopMovement();
        DisableInput(Cast<APlayerController>(playerController));
    }

    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    //MontageDurationは、0.fから算出した数値を後続の判定または計算に使います。
    float MontageDuration = 0.f;
    //「USkeletalMeshComponent* mesh = GetMesh()」が成立するとき、mesh->bPauseAnimsを更新します。
    if (USkeletalMeshComponent* mesh = GetMesh())
    {
        //以前の死亡処理で止めたPoseが残らないように戻します。
        mesh->bPauseAnims = false;
    }

    //「m_pDeathMontage」が成立するとき、StopAnimMontageを呼び出します。
    if (m_pDeathMontage)
    {
        StopAnimMontage();
        MontageDuration = PlayAnimMontage(m_pDeathMontage);
    }

    //「MontageDuration > 0.0f」が成立するとき、GetWorldTimerManagerを呼び出します。
    if (MontageDuration > 0.0f)
    {
        GetWorldTimerManager().SetTimer(m_deathPoseFreezeTimer, this, &APlayerChara::FreezeDeathPose, FMath::Max(0.05f, MontageDuration - 0.05f),
                                        false);
    }

    //通常は死亡アニメ最後のNotifyでこの処理を終了します.
    GetWorldTimerManager().SetTimer(m_deathTimer, this, &APlayerChara::FinishDeathSequence, FMath::Max(0.1f, MontageDuration) + m_deathScreenDelay,
                                    false);
}

//死亡Montageの最終姿勢を固定し、消える直前の立ち上がりを防ぎます。
void APlayerChara::FreezeDeathPose()
{
    //「!m_bIsDead」が成立するとき、続けて「USkeletalMeshComponent* mesh = GetMesh()」を判定します。
    if (!m_bIsDead) { return; }

    //「USkeletalMeshComponent* mesh = GetMesh()」が成立するとき、mesh->bPauseAnimsを更新します。
    if (USkeletalMeshComponent* mesh = GetMesh())
    {
        //死亡Montage終了直後にIdleへ戻って一瞬立ち上がるのを防ぎます。
        mesh->bPauseAnims = true;
    }
}

//死亡演出を終え、GameOver画面へ一度だけ遷移します。
void APlayerChara::FinishDeathSequence()
{
    //「m_bGameOverRequested」が成立するとき、m_bGameOverRequestedを更新します。
    if (m_bGameOverRequested) { return; }

    m_bGameOverRequested = true;
    GetWorldTimerManager().ClearTimer(m_deathTimer);
    GetWorldTimerManager().ClearTimer(m_deathPoseFreezeTimer);
    //Levelへ安全に遷移します。
    UGameplayStatics::OpenLevel(this, m_gameOverLevelName);
}

//アイテム取得。失敗時はfalseを返し、Pickupを消さない。

bool APlayerChara::PickUpItem(float Value, EItemType Type)
{
    //現在の状態に合う処理へ分けます。
    switch (Type)
    {
    case EItemType::EIT_Health: ++m_healItemCount; return true;

    case EItemType::EIT_Ammo:
        //「m_pPistolWeapon」が成立するとき、AddAmmoを呼び出します。
        if (m_pPistolWeapon)
        {
            m_pPistolWeapon->AddAmmo(Value);
            //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
            return true;
        }
        //呼び出し元へ失敗を返し、この関数でこれ以上の処理を行わないようにします。
        return false;

    case EItemType::EIT_ARAmmo:
        //「m_pARWeapon」が成立するとき、AddAmmoを呼び出します。
        if (m_pARWeapon)
        {
            m_pARWeapon->AddAmmo(Value);
            //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
            return true;
        }
        //呼び出し元へ失敗を返し、この関数でこれ以上の処理を行わないようにします。
        return false;

    case EItemType::EIT_WeaponAR:
        //「m_bHasAR && m_pARWeapon」が成立するとき、AddAmmoを呼び出します。
        if (m_bHasAR && m_pARWeapon)
        {
            //AR取得済みの場合、2個目以降のAR取得は無駄にせずAR弾薬として扱います。
            m_pARWeapon->AddAmmo(Value > 0.f ? Value : 30.f);
            //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
            return true;
        }

        if (!m_arWeaponClass || !GetWorld())
        {
            //呼び出し元へ失敗を返し、この関数でこれ以上の処理を行わないようにします。
            return false;
        }

        {
            //SpawnParamsは、Actor生成時の所有者や衝突時の生成規則を指定するために使います。
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = this;
            SpawnParams.Instigator = this;
            //ARは、GetWorld()->SpawnActor<AARWeapon>(m_arWeaponClass, FVector::ZeroVector,…から取得した参照を後続の呼び出しで使います。
            AARWeapon* AR = GetWorld()->SpawnActor<AARWeapon>(m_arWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
            if (!AR)
            {
                //呼び出し元へ失敗を返し、この関数でこれ以上の処理を行わないようにします。
                return false;
            }

            AR->SetOwnerCharacter(this);
            //「USkeletalMeshComponent* playerMesh = GetMesh()」が成立するとき、Rulesを呼び出します。
            if (USkeletalMeshComponent* playerMesh = GetMesh())
            {
                //Rulesは、Rulesの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
                const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);
                AR->AttachToComponent(playerMesh, Rules, WeaponSocketName);
            }
            m_pARWeapon = AR;
            m_pARWeapon->SetActorHiddenInGame(true);
            m_bHasAR = true;
            RebuildWeaponDisplayOrder();
            RefreshWeaponCarousel();
            //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
            return true;
        }

    default: return false;
    }
}

//取得した回復量を体力へ反映します。
void APlayerChara::ApplyHeal(float _amount)
{
    //「!m_bIsHealing || m_bIsDead」が成立するとき、GetWorldTimerManagerを呼び出します。
    if (!m_bIsHealing || m_bIsDead) { return; }

    GetWorldTimerManager().ClearTimer(m_healFallbackTimer);
    m_bIsHealing = false;
    m_Hp = FMath::Clamp(m_Hp + FMath::Max(0.f, _amount), 0.f, m_MaxHp);
    OnDamaged.Broadcast();
}

//回復アイテム使用

void APlayerChara::UseHealItem()
{
    //「m_healItemCount <= 0 || m_bIsHealing || m_bIsDead || m_bIsReloadingAnim || m_bIsSwitching…」が成立するとき、ResetIdleTimerを呼び出します。
    if (m_healItemCount <= 0 || m_bIsHealing || m_bIsDead || m_bIsReloadingAnim || m_bIsSwitchingWeapon || m_Hp >= m_MaxHp) { return; }

    ResetIdleTimer();
    m_bIsHealing = true;
    --m_healItemCount;

    //MontageDurationは、m_pHealMontage ? PlayAnimMontage(m_pHealMontage) : 0.fから算出した数値を後続の判定または計算に使います。
    const float MontageDuration = m_pHealMontage ? PlayAnimMontage(m_pHealMontage) : 0.f;
    GetWorldTimerManager().SetTimer(m_healFallbackTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { ApplyHeal(30.f); }),
                                    MontageDuration > 0.f ? MontageDuration : 0.1f, false);
}

//武器切り替え

void APlayerChara::PickupItem(EItemType _type, float _value) { (void)PickUpItem(_value, _type); }
