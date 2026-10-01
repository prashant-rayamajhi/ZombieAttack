#include "PlayerChara.h"
#include "../Weapon/GunWeapon.h"
#include "../Weapon/ARWeapon.h"
#include "../Weapon/MeleeWeapon.h"
#include "../Weapon/WeaponBase.h"
#include "../Components/PlayerAudio/PlayerAudioComponent.h"
#include "../Components/RifleAnimation/PlayerRifleAnimationComponent.h"
#include "../UI/Ammo/AmmoHUDWidget.h"
#include "../UI/PlayerUI/WeaponCarouselWidget.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

//装備中の武器で攻撃します。
void APlayerChara::Attack()
{
    if (!m_bCanControl || m_bIsDead || m_bIsHealing) { m_attackBuffer.Clear(); return; }
    const AGunWeapon* gun = Cast<AGunWeapon>(m_pCurrentWeapon);
    //動作完了直前の押下を拾うが、長時間の予約や複数発の蓄積はしない。
    if (m_bIsReloadingAnim || m_bIsSwitchingWeapon || (gun && !gun->CanFireNow()))
    {
        m_attackBuffer.Queue(static_cast<int32>(m_currentSlot), GetWorld()->GetTimeSeconds());
        return;
    }
    m_attackBuffer.Clear();

    ResetIdleTimer();

    //銃の弾が0の状態で攻撃ボタンを押した場合は、
    //GunWeapon側だけでReloadさせず、PlayerChara側のReloadWeaponを通します。
    //これにより、リロードMontageとリロード状態フラグが必ず動きます。
    if (m_currentSlot != EWeaponSlot::Knife)
    {
        //銃武器を保持します。
        AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon);
        if (gunWeapon && gunWeapon->GetCurrentAmmo() <= 0 && gunWeapon->CanReload())
        {
            ReloadWeapon();
            return;
        }

        //ARは構え終わる前に発砲しません。
        //初回入力でAimへ移行し、構え完了後の入力から弾を発射します。
        if (m_currentSlot == EWeaponSlot::AR && gunWeapon && gunWeapon->GetCurrentAmmo() > 0)
        {
            m_bRifleTriggerHeld = true;
            m_bPendingRifleShot = true;
            if (!IsRifleReadyToFire())
            {
                EnsurePersistentRifleAim();
                return;
            }

            FirePendingRifleShot();
            return;
        }
    }

    Super::Attack();
}

//移動処理

void APlayerChara::SwitchToPistol() { SwitchWeaponToSlot(EWeaponSlot::Pistol); }

//装備をARへ切り替えます。
void APlayerChara::SwitchToAR() { SwitchWeaponToSlot(EWeaponSlot::AR); }

//装備をKnifeへ切り替えます。
void APlayerChara::SwitchToKnife() { SwitchWeaponToSlot(EWeaponSlot::Knife); }

//WeaponAvailableかを判定します。
bool APlayerChara::IsWeaponAvailable(EWeaponSlot _slot) const
{
    switch (_slot)
    {
    case EWeaponSlot::Pistol: return IsValid(m_pPistolWeapon);
    case EWeaponSlot::AR: return m_bHasAR && IsValid(m_pARWeapon);
    case EWeaponSlot::Knife: return IsValid(m_pKnifeWeapon);
    default: return false;
    }
}

//WeaponForSlotを取得して呼び出し元へ返します。
AWeaponBase* APlayerChara::GetWeaponForSlot(EWeaponSlot _slot) const
{
    switch (_slot)
    {
    case EWeaponSlot::Pistol: return m_pPistolWeapon;
    case EWeaponSlot::AR: return m_pARWeapon;
    case EWeaponSlot::Knife: return m_pKnifeWeapon;
    default: return nullptr;
    }
}

//WeaponDisplayOrderを現在のデータから組み直します。
void APlayerChara::RebuildWeaponDisplayOrder()
{
    m_weaponDisplayOrder.Reset();
    const EWeaponSlot CanonicalOrder[] = {EWeaponSlot::Pistol, EWeaponSlot::AR, EWeaponSlot::Knife};
    for (const EWeaponSlot weaponSlot : CanonicalOrder)
    {
        if (IsWeaponAvailable(weaponSlot))
        {
            m_weaponDisplayOrder.Add(weaponSlot);
        }
    }

    MoveWeaponSlotToBottom(m_currentSlot);
}

//武器スロットToBottomを指定された位置へ移します。
void APlayerChara::MoveWeaponSlotToBottom(EWeaponSlot _slot)
{
    //番号を保持します。
    const int32 Index = m_weaponDisplayOrder.IndexOfByKey(_slot);
    if (Index == INDEX_NONE) { return; }

    m_weaponDisplayOrder.RemoveAt(Index);
    m_weaponDisplayOrder.Add(_slot);
}

//武器選択UIを現在の装備と進行状況に合わせて更新します。
void APlayerChara::RefreshWeaponCarousel()
{
    if (m_pWeaponCarousel)
    {
        m_pWeaponCarousel->RefreshWeaponOrder(m_weaponDisplayOrder, m_currentSlot);
    }
}

//NextWeaponを順番に切り替えます。
void APlayerChara::CycleNextWeapon()
{
    if (m_weaponDisplayOrder.Num() <= 1) { return; }

    //一番上の武器を選択し、選択された武器を一番下へ移動します。
    SwitchWeaponToSlot(m_weaponDisplayOrder[0]);
}

//PreviousWeaponを順番に切り替えます。
void APlayerChara::CyclePreviousWeapon()
{
    if (m_weaponDisplayOrder.Num() <= 1) { return; }

    //現在武器のすぐ上にある武器を選択します。
    SwitchWeaponToSlot(m_weaponDisplayOrder[m_weaponDisplayOrder.Num() - 2]);
}

//WeaponWheelが発生したときの処理を行います。
void APlayerChara::OnWeaponWheel(float _wheelValue)
{
    //現在のフレームで武器切替入力が押されているかを示します。
    const bool bPressedNow = FMath::Abs(_wheelValue) > 0.1f;
    //前のフレームでも武器切替入力が押されていたかを示します。
    const bool bWasPressed = FMath::Abs(m_lastWeaponWheelInput) > 0.1f;
    if (bPressedNow && !bWasPressed)
    {
        //UnrealのMouse Wheel Axisは、通常は下スクロールでマイナス値になります。
        if (_wheelValue < 0.f)
        {
            CycleNextWeapon();
        }
        else
        {
            CyclePreviousWeapon();
        }
    }
    m_lastWeaponWheelInput = _wheelValue;
}

//指定されたスロットの武器を装備し、MeshとHUDを更新します。
bool APlayerChara::SwitchWeaponToSlot(EWeaponSlot _slot)
{
    if (m_currentSlot == _slot || !IsWeaponAvailable(_slot) || m_bIsHealing || m_bIsDead || m_bIsReloadingAnim || m_bIsSwitchingWeapon)
    {
        //呼び出し元へ失敗を返し、この関数でこれ以上の処理を行わないようにします。
        return false;
    }

    //切り替えモンタージュを保持します。
    UAnimMontage* switchMontage = nullptr;
    switch (_slot)
    {
    case EWeaponSlot::Pistol: switchMontage = m_pSwitchToPistolMontage; break;
    case EWeaponSlot::AR: switchMontage = m_pSwitchToARMontage; break;
    case EWeaponSlot::Knife: switchMontage = m_pSwitchToKnifeMontage; break;
    }
    AWeaponBase* nextWeapon = GetWeaponForSlot(_slot);
    if (!nextWeapon) { return false; }

    //速度を返します。
    const bool bIsMoving = GetVelocity().SizeSquared2D() > FMath::Square(10.0f) || !m_charaMovement.IsNearlyZero(0.05f);

    ResetIdleTimer();
    if (m_pCurrentWeapon)
    {
        m_pCurrentWeapon->SetActorHiddenInGame(true);
    }

    m_currentSlot = _slot;
    //移動中は全身武器切替状態へ入れず、脚のLocomotionを継続します。
    m_bIsSwitchingWeapon = !bIsMoving;
    m_bIsFiringRifle = false;
    m_bRifleCombatAim = false;
    m_bPendingRifleShot = false;
    m_bRifleTriggerHeld = false;
    StopRifleAimPose(0.08f);
    GetWorldTimerManager().ClearTimer(m_rifleCombatAimTimer);
    GetWorldTimerManager().ClearTimer(m_rifleAutomaticFireTimer);
    nextWeapon->SetActorHiddenInGame(false);
    m_pCurrentWeapon = nextWeapon;
    m_pEquippedWeapon = nextWeapon;

    MoveWeaponSlotToBottom(_slot);
    RefreshWeaponCarousel();
    float switchDuration = 0.4f;
    if (switchMontage && !bIsMoving)
    {
        switchDuration = FMath::Max(PlayAnimMontage(switchMontage), 0.3f);
    }
    else if (bIsMoving)
    {
        //全身Montageを歩行中に重ねると足が止まって滑って見えるため、
        //移動中はLocomotionを維持し、武器の表示切替だけを短時間で完了します。
        switchDuration = 0.22f;
    }
    if (m_bIsSwitchingWeapon)
    {
        GetWorldTimerManager().SetTimer(m_switchWeaponTimer, this, &APlayerChara::OnSwitchWeaponFinished, switchDuration, false);
    }
    else
    {
        GetWorldTimerManager().ClearTimer(m_switchWeaponTimer);
    }
    if (m_pReloadUI)
    {
        m_pReloadUI->SetWeapon(Cast<AGunWeapon>(m_pCurrentWeapon));
    }

    //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
    return true;
}

//切り替え武器完了状態を処理します。
void APlayerChara::OnSwitchWeaponFinished() { m_bIsSwitchingWeapon = false; }

//リロード処理

bool APlayerChara::PlayKnifeAttackMontage(int32 _comboIndex)
{
    if (!m_knifeAttackMontages.IsValidIndex(_comboIndex)) { return false; }

    //モンタージュを保持します。
    UAnimMontage* montage = m_knifeAttackMontages[_comboIndex];
    if (!montage) { return false; }

    return PlayAnimMontage(montage) > 0.0f;
}

//リロード武器を処理します。
void APlayerChara::ReloadWeapon()
{
    if (m_currentSlot == EWeaponSlot::Knife || m_bIsReloadingAnim || m_bIsDead || m_bIsHealing || m_bIsSwitchingWeapon) { return; }

    //銃武器を保持します。
    AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon);
    if (!gunWeapon || !gunWeapon->CanReload()) { return; }

    ResetIdleTimer();
    m_bIsAiming = false;
    m_bIsFiringRifle = false;
    m_bRifleCombatAim = false;
    m_bPendingRifleShot = false;
    m_bRifleTriggerHeld = false;
    GetWorldTimerManager().ClearTimer(m_rifleAutomaticFireTimer);
    StopRifleAimPose(0.06f);
    if (m_pRifleAnimationComponent)
    {
        m_pRifleAnimationComponent->SetReloading(GetMesh());
    }
    m_bIsReloadingAnim = true;

    //メッシュを返します。
    UAnimInstance* animInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    //リロードアニメーションを保持します。
    UAnimSequenceBase* reloadAnimation = m_currentSlot == EWeaponSlot::Pistol ? m_pPistolReloadAnimation : m_pRifleReloadAnimation;
    UAnimMontage* playedMontage = nullptr;
    float reloadDuration = 0.0f;
    if (animInstance && reloadAnimation)
    {
        playedMontage = animInstance->PlaySlotAnimationAsDynamicMontage(reloadAnimation, TEXT("UpperBody"), 0.08f, 0.08f, 1.0f, 1, 0.0f);
        reloadDuration = reloadAnimation->GetPlayLength();
    }
    else if (m_pReloadMontage)
    {
        reloadDuration = PlayAnimMontage(m_pReloadMontage);
        playedMontage = m_pReloadMontage;
    }
    if (!playedMontage || reloadDuration <= KINDA_SMALL_NUMBER)
    {
        m_bIsReloadingAnim = false;
        StopRifleAimPose(0.0f);
        return;
    }

    gunWeapon->Reload();
    FOnMontageEnded montageEndedDelegate;
    montageEndedDelegate.BindUObject(this, &APlayerChara::HandleReloadMontageEnded);
    animInstance->Montage_SetEndDelegate(montageEndedDelegate, playedMontage);
    if (m_pAudioComponent)
    {
        m_pAudioComponent->PlayReloadSound(gunWeapon, reloadDuration);
    }
}

//ARの構えモーションを先に再生し、完了するまで発砲を禁止します。
void APlayerChara::BeginRifleAimTransition()
{
    //ライフルエイム状態を返します。
    if (!ShouldUsePersistentRifleAim() || GetRifleAimState() == ERifleAimState::Raising || GetRifleAimState() == ERifleAimState::Ready) { return; }

    m_bIsFiringRifle = true;
    m_bRifleCombatAim = false;
    if (m_pRifleAnimationComponent)
    {
        m_pRifleAnimationComponent->BeginAimSequence(GetMesh());
    }
    else
    {
        //Componentがない派生BPでも戦闘不能にならないようにします。
        HandleRifleAimReady();
    }
    SwapCrosshairWidget();
}

//RifleAnimationComponentがAim Idleへ移った後に戦闘状態を確定します。
void APlayerChara::HandleRifleAimReady()
{
    if (!ShouldUsePersistentRifleAim() || (!m_bRifleTriggerHeld && !m_bIsAiming))
    {
        m_bIsFiringRifle = false;
        StopRifleAimPose();
        return;
    }

    m_bIsFiringRifle = false;
    m_bRifleCombatAim = true;
    RefreshRifleCombatAim();
    FirePendingRifleShot();
    SwapCrosshairWidget();
}

//照準完了まで保留したARの一発を発射します。
void APlayerChara::FirePendingRifleShot()
{
    //Readyフラグだけではなく、Aiming Idleの実再生を確認してから発砲します。
    if (!m_bPendingRifleShot || !m_bRifleTriggerHeld || !IsRifleReadyToFire()) { return; }

    m_bPendingRifleShot = false;
    //銃武器を保持します。
    AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon);
    //現在の弾薬を返します。
    if (m_currentSlot != EWeaponSlot::AR || !gunWeapon || gunWeapon->GetCurrentAmmo() <= 0) { return; }

    //Hip Fireでも、Aim遷移が完了してから一発だけ発射します。
    //入力処理がTickより先に来ても、発射フレームで身体と銃を中央へ合わせます。
    AlignRifleVisualFacing();
    gunWeapon->UseWeapon();
    RefreshRifleCombatAim();
    if (gunWeapon->GetCurrentAmmo() <= 0)
    {
        FinishRifleAttack();
        return;
    }

    GetWorldTimerManager().SetTimer(m_rifleAutomaticFireTimer, this, &APlayerChara::ContinueRifleAutomaticFire, gunWeapon->GetFireRate(), false);
}

//ARのトリガー保持中だけ、Aiming Idleを維持したまま次弾を発射します。
void APlayerChara::ContinueRifleAutomaticFire()
{
    if (!m_bRifleTriggerHeld || !IsRifleReadyToFire())
    {
        FinishRifleAttack();
        return;
    }

    //銃武器を保持します。
    AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon);
    if (!gunWeapon || gunWeapon->GetCurrentAmmo() <= 0)
    {
        FinishRifleAttack();
        return;
    }

    m_bPendingRifleShot = true;
    FirePendingRifleShot();
}

//射撃入力終了または弾切れでAimループを止め、Rifle Idleへ戻します。
void APlayerChara::FinishRifleAttack()
{
    GetWorldTimerManager().ClearTimer(m_rifleAutomaticFireTimer);
    m_bRifleTriggerHeld = false;
    m_bPendingRifleShot = false;
    m_bIsFiringRifle = false;
    m_bRifleCombatAim = false;

    //射撃専用の中央向き補正を残さず、通常のIdle・移動姿勢へ戻します。
    if (USkeletalMeshComponent* playerMesh = GetMesh())
    {
        playerMesh->SetRelativeRotation(m_defaultMeshRelativeRotation);
    }

    StopRifleAimPose(0.12f);
    SwapCrosshairWidget();
}

//攻撃を停止し、関連する状態を解除します。
void APlayerChara::StopAttack()
{
    if (m_currentSlot == EWeaponSlot::AR)
    {
        //ARはボタンを離した後に予約弾を発射しない。
        m_attackBuffer.Clear();
        FinishRifleAttack();
    }
}

//動作が終わったフレームで短い射撃予約を回収し、押し直しの必要を減らす。
void APlayerChara::UpdateBufferedAttack()
{
    if (!m_bCanControl || m_bIsDead || m_bIsHealing) { m_attackBuffer.Clear(); return; }
    if (m_bIsReloadingAnim || m_bIsSwitchingWeapon) { return; }
    const AGunWeapon* gun = Cast<AGunWeapon>(m_pCurrentWeapon);
    if (gun && !gun->CanFireNow()) { return; }
    if (m_attackBuffer.Consume(static_cast<int32>(m_currentSlot), GetWorld()->GetTimeSeconds())) { Attack(); }
}

//アニメーションの通知を受け、ライフルのエイム移行を完了します。
void APlayerChara::FinishRifleAimTransitionFromAnimation()
{
    if (m_pRifleAnimationComponent)
    {
        m_pRifleAnimationComponent->CompleteAimFromNotify();
    }
}

//ARの構え遷移を開始できる状態か判定します。
bool APlayerChara::ShouldUsePersistentRifleAim() const
{
    return m_currentSlot == EWeaponSlot::AR && IsValid(m_pARWeapon) && !m_bIsDead && !m_bIsHealing && !m_bIsReloadingAnim && !m_bIsSwitchingWeapon;
}

//PersistentARの照準が利用できる状態を保証します。
void APlayerChara::EnsurePersistentRifleAim()
{
    if (!ShouldUsePersistentRifleAim()) { return; }

    //Montageが外部処理で中断された場合は、Ready表示だけを信用せず
    //構え遷移からやり直してアイドル射撃を防ぎます。
    if (GetRifleAimState() == ERifleAimState::Ready && !IsRifleReadyToFire())
    {
        StopRifleAimPose(0.0f);
    }

    //ライフルエイム状態を返します。
    if (GetRifleAimState() == ERifleAimState::Inactive)
    {
        BeginRifleAimTransition();
    }
}

//ARが照準姿勢へ到達し、発射可能か判定します。
bool APlayerChara::IsRifleReadyToFire() const { return m_pRifleAnimationComponent && m_pRifleAnimationComponent->IsAimReadyPoseActive(); }

//エイム中かどうかを返します。
bool APlayerChara::IsAiming() const
{
    return m_bIsAiming ||
           //ライフルエイム状態を返します。
           (ShouldUsePersistentRifleAim() && (GetRifleAimState() == ERifleAimState::Raising || GetRifleAimState() == ERifleAimState::Ready));
}

//ライフルエイム状態を返します。
ERifleAimState APlayerChara::GetRifleAimState() const
{
    return m_pRifleAnimationComponent ? m_pRifleAnimationComponent->GetAimState() : ERifleAimState::Inactive;
}

//射撃入力と装備状態からARの上半身照準を更新します。
void APlayerChara::RefreshRifleCombatAim()
{
    if (m_currentSlot != EWeaponSlot::AR) { return; }

    m_bRifleCombatAim = true;
    GetWorldTimerManager().ClearTimer(m_rifleCombatAimTimer);
    SwapCrosshairWidget();
}

//射撃終了後にライフルの構えを解除します。
void APlayerChara::EndRifleCombatAim()
{
    if (ShouldUsePersistentRifleAim())
    {
        FinishRifleAttack();
        return;
    }

    m_bRifleCombatAim = false;
    StopRifleAimPose();
    SwapCrosshairWidget();
}

//ARの照準Poseを停止し、関連する状態を解除します。
void APlayerChara::StopRifleAimPose(float _blendOutTime)
{
    if (m_pRifleAnimationComponent)
    {
        m_pRifleAnimationComponent->StopAimSequence(_blendOutTime);
    }
}

//ReloadMontageEndedの通知を受けてゲーム状態へ反映します。
void APlayerChara::HandleReloadMontageEnded(UAnimMontage* _montage, bool _bInterrupted)
{
    (void)_montage;
    if (_bInterrupted)
    {
        if (AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon))
        {
            gunWeapon->CancelReload();
        }
        if (m_pAudioComponent)
        {
            m_pAudioComponent->StopReloadSound();
        }
        m_bIsReloadingAnim = false;
        StopRifleAimPose(0.0f);
        return;
    }

    FinishReloadFromAnimation();
}

//アニメーションの通知を受け、リロードを完了します。
void APlayerChara::FinishReloadFromAnimation()
{
    if (!m_bIsReloadingAnim) { return; }
    if (AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon))
    {
        gunWeapon->FinishReload();
    }
    if (m_pAudioComponent)
    {
        m_pAudioComponent->StopReloadSound();
    }

    m_bIsReloadingAnim = false;
    StopRifleAimPose(0.0f);
}

//着地処理

void APlayerChara::AttachWeapon(TSubclassOf<AActor> _weaponClass)
{
    if (!_weaponClass) { return; }
    if (m_pEquippedWeapon)
    {
        m_pEquippedWeapon->Destroy();
        m_pEquippedWeapon = nullptr;
    }
    FActorSpawnParameters spawnParams;
    spawnParams.Owner = this;
    spawnParams.Instigator = GetInstigator();
    //ワールドを返します。
    AWeaponBase* spawnedWeapon = GetWorld()->SpawnActor<AWeaponBase>(_weaponClass, FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);
    if (!spawnedWeapon) { return; }
    spawnedWeapon->SetOwnerCharacter(this);
    spawnedWeapon->SetOwner(this);
    if (auto* mesh = GetMesh())
    {
        FAttachmentTransformRules attachRules(EAttachmentRule::KeepRelative, true);
        spawnedWeapon->AttachToComponent(mesh, attachRules, m_weaponSocketName);
    }
    m_pEquippedWeapon = spawnedWeapon;
    m_pCurrentWeapon = spawnedWeapon;
}

//カメラ回転処理

void APlayerChara::StartAim()
{
    if (m_bIsAiming || m_bIsDead || m_bIsHealing || m_bIsReloadingAnim || m_bIsSwitchingWeapon) { return; }
    ResetIdleTimer();
    m_bIsAiming = true;
    GetWorldTimerManager().ClearTimer(m_rifleCombatAimTimer);
    if (m_currentSlot == EWeaponSlot::AR && !m_bRifleCombatAim && !m_bIsFiringRifle)
    {
        BeginRifleAimTransition();
    }
    SwapCrosshairWidget();
}

//エイム終了

void APlayerChara::StopAim()
{
    if (!m_bIsAiming) { return; }
    m_bIsAiming = false;
    if (m_currentSlot == EWeaponSlot::AR)
    {
        if (!m_bRifleTriggerHeld)
        {
            FinishRifleAttack();
        }
    }
    else
    {
        m_bIsFiringRifle = false;
        m_bPendingRifleShot = false;
        EndRifleCombatAim();
    }
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
    SwapCrosshairWidget();
}

//クロスヘアウィジェットの切り替え
