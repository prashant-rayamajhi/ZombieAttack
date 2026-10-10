#include "PlayerChara.h"
#include "../Components/PlayerAudio/PlayerAudioComponent.h"
#include "../Weapon/ARWeapon.h"
#include "../Weapon/GunWeapon.h"
#include "../Weapon/MeleeWeapon.h"
#include "../Weapon/WeaponBase.h"
#include "../Components/RifleAnimation/PlayerRifleAnimationComponent.h"
#include "../UI/Ammo/AmmoHUDWidget.h"
#include "../UI/PlayerUI/CombatCrosshairWidget.h"
#include "../UI/PlayerUI/WeaponCarouselWidget.h"
#include "../UI/EnemyUI/EnemyLocatorWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

//受けたダメージを体力へ反映します。
float APlayerChara::TakeDamage(float _damageAmount, FDamageEvent const& _damageEvent, AController* _eventInstigator, AActor* _damageCauser)
{
    if (m_bIsDead || _damageAmount <= 0.f) { return 0.f; }

    //ダメージを保持します。
    const float damage = Super::TakeDamage(_damageAmount, _damageEvent, _eventInstigator, _damageCauser);
    if (damage <= 0.f) { return 0.f; }

    m_hp = FMath::Clamp(m_hp - damage, 0.f, m_maxHp);
    m_onDamaged.Broadcast();
    if (m_hp <= 0.f)
    {
        BeginDeathSequence();
    }
    return damage;
}

//KillHitStopを再生します。
void APlayerChara::PlayKillHitStop()
{
    //ワールドを返します。
    UWorld* world = GetWorld();
    if (!world || m_bIsDead || m_killHitStopDuration <= 0.0f || m_killHitStopTimeDilation >= 1.0f) { return; }

    //ARでまとめて倒しても停止を延長せず、次の照準操作へ戻れる間を残す。
    const double now = world->GetRealTimeSeconds();
    if (m_killStopActive || now - m_lastKillStop < 0.35) { return; }
    m_lastKillStop = now;
    m_beforeKillStop = UGameplayStatics::GetGlobalTimeDilation(world);
    m_killStopActive = true;
    UGameplayStatics::SetGlobalTimeDilation(world, m_beforeKillStop * FMath::Clamp(m_killHitStopTimeDilation, 0.01f, 1.0f));

    //WorldTimerはゲーム時間で進むため、実時間の長さになるようDilationを掛けます。
    const float timerDuration = m_killHitStopDuration * UGameplayStatics::GetGlobalTimeDilation(world);
    GetWorldTimerManager().SetTimer(m_killHitStopTimer, this, &APlayerChara::RestoreKillHitStop, timerDuration, false);
}

//KillHitStopを変更前の状態へ戻します。
void APlayerChara::RestoreKillHitStop()
{
    if (!m_killStopActive) { return; }
    if (UWorld* world = GetWorld())
    {
        const float expected = m_beforeKillStop * FMath::Clamp(m_killHitStopTimeDilation, 0.01f, 1.0f);
        //別の演出が倍率を変更済みなら上書きせず、自分の停止分だけを戻す。
        if (FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(world), expected))
        {
            UGameplayStatics::SetGlobalTimeDilation(world, m_beforeKillStop);
        }
        GetWorldTimerManager().ClearTimer(m_killHitStopTimer);
    }
    m_killStopActive = false;
}

//レベル遷移や破棄で終了タイマーが消えても、時間倍率を残さない。
void APlayerChara::EndPlay(const EEndPlayReason::Type _reason)
{
    RestoreKillHitStop();
    m_attackBuffer.Clear();
    //同じワールド内でPawnを再生成しても、旧Pawnの予約攻撃や回復を実行しない。
    GetWorldTimerManager().ClearAllTimersForObject(this);
    if (m_pKnifeWeapon) { m_pKnifeWeapon->ResetCombo(); }
    if (m_pAudioComponent) { m_pAudioComponent->StopReloadSound(0.0f); }
    if (m_pRifleAnimationComponent)
    {
        m_pRifleAnimationComponent->OnAimReady().RemoveAll(this);
        m_pRifleAnimationComponent->StopAimSequence(0.0f);
    }
    //Viewportへの登録はPawnと別の寿命を持つため、五種類のHUDを明示的に取り外す。
    m_onDamaged.Clear();
    if (m_pReloadUI) { m_pReloadUI->SetWeapon(nullptr); }
    UUserWidget* widgets[] = {m_pUserWidget, m_pPlayerHp, m_pReloadUI, m_pWeaponCarousel, m_pEnemyLocatorWidget};
    for (UUserWidget* widget : widgets)
    {
        if (IsValid(widget)) { widget->RemoveFromParent(); }
    }
    m_pUserWidget = nullptr;
    m_pPlayerHp = nullptr;
    m_pReloadUI = nullptr;
    m_pWeaponCarousel = nullptr;
    m_pEnemyLocatorWidget = nullptr;
    //AttachやOwner指定だけでは武器Actorは破棄されない。重複する装備参照をまとめて一度ずつ破棄する。
    TSet<AActor*> weapons = {m_pPistolWeapon, m_pKnifeWeapon, m_pARWeapon, m_pCurrentWeapon.Get(), m_pEquippedWeapon.Get()};
    for (AActor* weapon : weapons)
    {
        if (IsValid(weapon) && weapon->GetOwner() == this) { weapon->Destroy(); }
    }
    m_pPistolWeapon = nullptr;
    m_pKnifeWeapon = nullptr;
    m_pARWeapon = nullptr;
    m_pCurrentWeapon = nullptr;
    m_pEquippedWeapon = nullptr;
    Super::EndPlay(_reason);
}

//死亡Sequenceを開始するための状態を設定します。
void APlayerChara::BeginDeathSequence()
{
    if (m_bIsDead) { return; }

    m_bIsDead = true;
    RestoreKillHitStop();
    m_attackBuffer.Clear();
    m_bCanControl = false;
    m_charaMovement = FVector2D::ZeroVector;
    m_bIsAiming = false;
    m_bIsFiringRifle = false;
    m_bRifleCombatAim = false;
    m_bPendingRifleShot = false;
    StopRifleAimPose(0.0f);
    m_bIsHealing = false;
    if (AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon))
    {
        gunWeapon->CancelReload();
    }
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
    if (AController* playerController = GetController())
    {
        playerController->StopMovement();
        DisableInput(Cast<APlayerController>(playerController));
    }

    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    float MontageDuration = 0.f;
    if (USkeletalMeshComponent* mesh = GetMesh())
    {
        //以前の死亡処理で止めたPoseが残らないように戻します。
        mesh->bPauseAnims = false;
    }
    if (m_pDeathMontage)
    {
        StopAnimMontage();
        MontageDuration = PlayAnimMontage(m_pDeathMontage);
    }
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
    if (!m_bIsDead) { return; }
    if (USkeletalMeshComponent* mesh = GetMesh())
    {
        //死亡Montage終了直後にIdleへ戻って一瞬立ち上がるのを防ぎます。
        mesh->bPauseAnims = true;
    }
}

//死亡演出を終え、GameOver画面へ一度だけ遷移します。
void APlayerChara::FinishDeathSequence()
{
    if (m_bGameOverRequested) { return; }

    m_bGameOverRequested = true;
    GetWorldTimerManager().ClearTimer(m_deathTimer);
    GetWorldTimerManager().ClearTimer(m_deathPoseFreezeTimer);
    //Levelへ安全に遷移します。
    UGameplayStatics::OpenLevel(this, m_gameOverLevelName);
}

//アイテム取得。失敗時はfalseを返し、Pickupを消さない。

bool APlayerChara::PickUpItem(float _value, EItemType _type)
{
    switch (_type)
    {
    case EItemType::EIT_Health: ++m_healItemCount; return true;

    case EItemType::EIT_Ammo:
        if (m_pPistolWeapon)
        {
            m_pPistolWeapon->AddAmmo(_value);
            //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
            return true;
        }
        //呼び出し元へ失敗を返し、この関数でこれ以上の処理を行わないようにします。
        return false;

    case EItemType::EIT_ARAmmo:
        if (m_pARWeapon)
        {
            m_pARWeapon->AddAmmo(_value);
            //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
            return true;
        }
        //呼び出し元へ失敗を返し、この関数でこれ以上の処理を行わないようにします。
        return false;

    case EItemType::EIT_WeaponAR:
        if (m_bHasAR && m_pARWeapon)
        {
            //AR取得済みの場合、2個目以降のAR取得は無駄にせずAR弾薬として扱います。
            m_pARWeapon->AddAmmo(_value > 0.f ? _value : 30.f);
            //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
            return true;
        }

        if (!m_arWeaponClass || !GetWorld())
        {
            //呼び出し元へ失敗を返し、この関数でこれ以上の処理を行わないようにします。
            return false;
        }

        {
            FActorSpawnParameters spawnParams;
            spawnParams.Owner = this;
            spawnParams.Instigator = this;
            AARWeapon* arWeapon = GetWorld()->SpawnActor<AARWeapon>(m_arWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);
            if (!arWeapon)
            {
                //呼び出し元へ失敗を返し、この関数でこれ以上の処理を行わないようにします。
                return false;
            }

            arWeapon->SetOwnerCharacter(this);
            if (USkeletalMeshComponent* playerMesh = GetMesh())
            {
                const FAttachmentTransformRules attachRules(EAttachmentRule::SnapToTarget, true);
                arWeapon->AttachToComponent(playerMesh, attachRules, m_weaponSocketName);
            }
            m_pARWeapon = arWeapon;
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
    if (!m_bIsHealing || m_bIsDead) { return; }

    GetWorldTimerManager().ClearTimer(m_healFallbackTimer);
    m_bIsHealing = false;
    m_hp = FMath::Clamp(m_hp + FMath::Max(0.f, _amount), 0.f, m_maxHp);
    m_onDamaged.Broadcast();
}

//回復アイテム使用

void APlayerChara::UseHealItem()
{
    if (m_healItemCount <= 0 || m_bIsHealing || m_bIsDead || m_bIsReloadingAnim || m_bIsSwitchingWeapon || m_hp >= m_maxHp) { return; }

    ResetIdleTimer();
    m_bIsHealing = true;
    --m_healItemCount;
    const float MontageDuration = m_pHealMontage ? PlayAnimMontage(m_pHealMontage) : 0.f;
    GetWorldTimerManager().SetTimer(m_healFallbackTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { ApplyHeal(30.f); }),
                                    MontageDuration > 0.f ? MontageDuration : 0.1f, false);
}

//武器切り替え

void APlayerChara::PickupItem(EItemType _type, float _value) { (void)PickUpItem(_value, _type); }
