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
    //「!m_bCanControl || m_bIsDead || m_bIsHealing || m_bIsReloadingAnim || m_bIsSwitchingWeapon」が成立するとき、ResetIdleTimerを呼び出します。
    if (!m_bCanControl || m_bIsDead || m_bIsHealing || m_bIsReloadingAnim || m_bIsSwitchingWeapon) { return; }

    ResetIdleTimer();

    //銃の弾が0の状態で攻撃ボタンを押した場合は、
    //GunWeapon側だけでReloadさせず、PlayerChara側のReloadWeaponを通します。
    //これにより、リロードMontageとリロード状態フラグが必ず動きます。
    if (m_currentSlot != EWeaponSlot::Knife)
    {
        //銃武器を保持します。
        AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon);
        //「gunWeapon && gunWeapon->GetCurrentAmmo() <= 0 && gunWeapon->CanReload()」が成立するとき、ReloadWeaponを呼び出します。
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

            //「!IsRifleReadyToFire()」が成立するとき、EnsurePersistentRifleAimを呼び出します。
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
    //現在の状態に合う処理へ分けます。
    switch (_slot)
    {
    //IsValidは、名前が示す条件の成立可否を呼び出し元へ返します。
    case EWeaponSlot::Pistol: return IsValid(m_pPistolWeapon);
    //IsValidは、名前が示す条件の成立可否を呼び出し元へ返します。
    case EWeaponSlot::AR: return m_bHasAR && IsValid(m_pARWeapon);
    //IsValidは、名前が示す条件の成立可否を呼び出し元へ返します。
    case EWeaponSlot::Knife: return IsValid(m_pKnifeWeapon);
    default: return false;
    }
}

//WeaponForSlotを取得して呼び出し元へ返します。
AWeaponBase* APlayerChara::GetWeaponForSlot(EWeaponSlot _slot) const
{
    //現在の状態に合う処理へ分けます。
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

    //「const EWeaponSlot weaponSlot : CanonicalOrder」の範囲を走査し、続けて「IsWeaponAvailable(weaponSlot)」を判定します。
    for (const EWeaponSlot weaponSlot : CanonicalOrder)
    {
        //「IsWeaponAvailable(weaponSlot)」が成立するとき、Addを呼び出します。
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
    //「Index == INDEX_NONE」が成立するとき、RemoveAtを呼び出します。
    if (Index == INDEX_NONE) { return; }

    m_weaponDisplayOrder.RemoveAt(Index);
    m_weaponDisplayOrder.Add(_slot);
}

//武器選択UIを現在の装備と進行状況に合わせて更新します。
void APlayerChara::RefreshWeaponCarousel()
{
    //「m_pWeaponCarousel」が成立するとき、RefreshWeaponOrderを呼び出します。
    if (m_pWeaponCarousel)
    {
        m_pWeaponCarousel->RefreshWeaponOrder(m_weaponDisplayOrder, m_currentSlot);
    }
}

//NextWeaponを順番に切り替えます。
void APlayerChara::CycleNextWeapon()
{
    //「m_weaponDisplayOrder.Num() <= 1」が成立するとき、SwitchWeaponToSlotを呼び出します。
    if (m_weaponDisplayOrder.Num() <= 1) { return; }

    //一番上の武器を選択し、選択された武器を一番下へ移動します。
    SwitchWeaponToSlot(m_weaponDisplayOrder[0]);
}

//PreviousWeaponを順番に切り替えます。
void APlayerChara::CyclePreviousWeapon()
{
    //「m_weaponDisplayOrder.Num() <= 1」が成立するとき、SwitchWeaponToSlotを呼び出します。
    if (m_weaponDisplayOrder.Num() <= 1) { return; }

    //現在武器のすぐ上にある武器を選択します。
    SwitchWeaponToSlot(m_weaponDisplayOrder[m_weaponDisplayOrder.Num() - 2]);
}

//WeaponWheelが発生したときの処理を行います。
void APlayerChara::OnWeaponWheel(float Value)
{
    //現在のフレームで武器切替入力が押されているかを示します。
    const bool bPressedNow = FMath::Abs(Value) > 0.1f;
    //前のフレームでも武器切替入力が押されていたかを示します。
    const bool bWasPressed = FMath::Abs(m_lastWeaponWheelInput) > 0.1f;
    //「bPressedNow && !bWasPressed」が成立するとき、続けて「Value < 0.f」を判定します。
    if (bPressedNow && !bWasPressed)
    {
        //UnrealのMouse Wheel Axisは、通常は下スクロールでマイナス値になります。
        if (Value < 0.f)
        {
            CycleNextWeapon();
        }
        else
        {
            CyclePreviousWeapon();
        }
    }
    m_lastWeaponWheelInput = Value;
}

//指定されたスロットの武器を装備し、MeshとHUDを更新します。
bool APlayerChara::SwitchWeaponToSlot(EWeaponSlot _slot)
{
    //「m_currentSlot == _slot || !IsWeaponAvailable(_slot) || m_bIsHealing || m_bIsDead || m_bIs…」が成立するとき、この関数を終了します。
    if (m_currentSlot == _slot || !IsWeaponAvailable(_slot) || m_bIsHealing || m_bIsDead || m_bIsReloadingAnim || m_bIsSwitchingWeapon)
    {
        //呼び出し元へ失敗を返し、この関数でこれ以上の処理を行わないようにします。
        return false;
    }

    //切り替えモンタージュを保持します。
    UAnimMontage* switchMontage = nullptr;
    //現在の状態に合う処理へ分けます。
    switch (_slot)
    {
    case EWeaponSlot::Pistol: switchMontage = m_pSwitchToPistolMontage; break;
    case EWeaponSlot::AR: switchMontage = m_pSwitchToARMontage; break;
    case EWeaponSlot::Knife: switchMontage = m_pSwitchToKnifeMontage; break;
    }

    //nextWeaponは、GetWeaponForSlot(_slot)から取得した参照を後続の呼び出しで使います。
    AWeaponBase* nextWeapon = GetWeaponForSlot(_slot);
    //「!nextWeapon」が成立するとき、GetVelocityを呼び出します。
    if (!nextWeapon) { return false; }

    //速度を返します。
    const bool b_mMoving = GetVelocity().SizeSquared2D() > FMath::Square(10.0f) || !m_charaMovement.IsNearlyZero(0.05f);

    ResetIdleTimer();
    //「m_pCurrentWeapon」が成立するとき、SetActorHiddenInGameを呼び出します。
    if (m_pCurrentWeapon)
    {
        m_pCurrentWeapon->SetActorHiddenInGame(true);
    }

    m_currentSlot = _slot;
    //移動中は全身武器切替状態へ入れず、脚のLocomotionを継続します。
    m_bIsSwitchingWeapon = !b_mMoving;
    m_bIsFiringRifle = false;
    m_bRifleCombatAim = false;
    m_bPendingRifleShot = false;
    m_bRifleTriggerHeld = false;
    StopRifleAimPose(0.08f);
    GetWorldTimerManager().ClearTimer(m_rifleCombatAimTimer);
    GetWorldTimerManager().ClearTimer(m_rifleAutomaticFireTimer);
    nextWeapon->SetActorHiddenInGame(false);
    m_pCurrentWeapon = nextWeapon;
    EquippedWeapon = nextWeapon;

    MoveWeaponSlotToBottom(_slot);
    RefreshWeaponCarousel();

    //switchDurationは、0.4fから算出した数値を後続の判定または計算に使います。
    float switchDuration = 0.4f;
    //「switchMontage && !b_mMoving」が成立するとき、Maxを呼び出します。
    if (switchMontage && !b_mMoving)
    {
        switchDuration = FMath::Max(PlayAnimMontage(switchMontage), 0.3f);
    }
    else if (b_mMoving)
    {
        //全身Montageを歩行中に重ねると足が止まって滑って見えるため、
        //移動中はLocomotionを維持し、武器の表示切替だけを短時間で完了します。
        switchDuration = 0.22f;
    }

    //「m_bIsSwitchingWeapon」が成立するとき、GetWorldTimerManagerを呼び出します。
    if (m_bIsSwitchingWeapon)
    {
        GetWorldTimerManager().SetTimer(m_switchWeaponTimer, this, &APlayerChara::OnSwitchWeaponFinished, switchDuration, false);
    }
    else
    {
        GetWorldTimerManager().ClearTimer(m_switchWeaponTimer);
    }

    //「m_pReloadUI」が成立するとき、SetWeaponを呼び出します。
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
    //「!m_knifeAttackMontages.IsValidIndex(_comboIndex)」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (!m_knifeAttackMontages.IsValidIndex(_comboIndex)) { return false; }

    //モンタージュを保持します。
    UAnimMontage* montage = m_knifeAttackMontages[_comboIndex];
    //「!montage」が成立するとき、この関数を終了します。
    if (!montage) { return false; }

    return PlayAnimMontage(montage) > 0.0f;
}

//リロード武器を処理します。
void APlayerChara::ReloadWeapon()
{
    //「m_currentSlot == EWeaponSlot::Knife || m_bIsReloadingAnim || m_bIsDead || m_bIsHealing ||…」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (m_currentSlot == EWeaponSlot::Knife || m_bIsReloadingAnim || m_bIsDead || m_bIsHealing || m_bIsSwitchingWeapon) { return; }

    //銃武器を保持します。
    AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon);
    //「!gunWeapon || !gunWeapon->CanReload()」が成立するとき、ResetIdleTimerを呼び出します。
    if (!gunWeapon || !gunWeapon->CanReload()) { return; }

    ResetIdleTimer();
    m_bIsAiming = false;
    m_bIsFiringRifle = false;
    m_bRifleCombatAim = false;
    m_bPendingRifleShot = false;
    m_bRifleTriggerHeld = false;
    GetWorldTimerManager().ClearTimer(m_rifleAutomaticFireTimer);
    StopRifleAimPose(0.06f);
    //「m_pRifleAnimationComponent」が成立するとき、SetReloadingを呼び出します。
    if (m_pRifleAnimationComponent)
    {
        m_pRifleAnimationComponent->SetReloading(GetMesh());
    }
    m_bIsReloadingAnim = true;

    //メッシュを返します。
    UAnimInstance* animInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    //リロードアニメーションを保持します。
    UAnimSequenceBase* reloadAnimation = m_currentSlot == EWeaponSlot::Pistol ? m_pPistolReloadAnimation : m_pRifleReloadAnimation;

    //playedMontageは、nullptrから取得した参照を後続の呼び出しで使います。
    UAnimMontage* playedMontage = nullptr;
    //reloadDurationは、0.0fから算出した数値を後続の判定または計算に使います。
    float reloadDuration = 0.0f;
    //「animInstance && reloadAnimation」が成立するとき、PlaySlotAnimationAsDynamicMontageを呼び出します。
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

    //「!playedMontage || reloadDuration <= KINDA_SMALL_NUMBER」が成立するとき、m_bIsReloadingAnimを更新します。
    if (!playedMontage || reloadDuration <= KINDA_SMALL_NUMBER)
    {
        m_bIsReloadingAnim = false;
        StopRifleAimPose(0.0f);
        return;
    }

    gunWeapon->Reload();

    //montageEndedDelegateは、直後の初期化結果を、同じスコープ内でこの名前を参照する計算や関数呼び出しへ渡すために使います。
    FOnMontageEnded montageEndedDelegate;
    montageEndedDelegate.BindUObject(this, &APlayerChara::HandleReloadMontageEnded);
    animInstance->Montage_SetEndDelegate(montageEndedDelegate, playedMontage);

    //「m_pAudioComponent」が成立するとき、PlayReloadSoundを呼び出します。
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

    //「m_pRifleAnimationComponent」が成立するとき、BeginAimSequenceを呼び出します。
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
    //「!ShouldUsePersistentRifleAim() || (!m_bRifleTriggerHeld && !m_bIsAiming)」が成立するとき、m_bIsFiringRifleを更新します。
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

    //「gunWeapon->GetCurrentAmmo() <= 0」が成立するとき、FinishRifleAttackを呼び出します。
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
    //「!m_bRifleTriggerHeld || !IsRifleReadyToFire()」が成立するとき、FinishRifleAttackを呼び出します。
    if (!m_bRifleTriggerHeld || !IsRifleReadyToFire())
    {
        FinishRifleAttack();
        return;
    }

    //銃武器を保持します。
    AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon);
    //「!gunWeapon || gunWeapon->GetCurrentAmmo() <= 0」が成立するとき、FinishRifleAttackを呼び出します。
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
    //「m_currentSlot == EWeaponSlot::AR」が成立するとき、FinishRifleAttackを呼び出します。
    if (m_currentSlot == EWeaponSlot::AR)
    {
        FinishRifleAttack();
    }
}

//アニメーションの通知を受け、ライフルのエイム移行を完了します。
void APlayerChara::FinishRifleAimTransitionFromAnimation()
{
    //「m_pRifleAnimationComponent」が成立するとき、CompleteAimFromNotifyを呼び出します。
    if (m_pRifleAnimationComponent)
    {
        m_pRifleAnimationComponent->CompleteAimFromNotify();
    }
}

//ARの構え遷移を開始できる状態か判定します。
bool APlayerChara::ShouldUsePersistentRifleAim() const
{
    //m_currentSlotは、= EWeaponSlot::AR && IsValid(m_pARWeapon) && !m_bIsDead && !m_bIsHealin…から構築した結果を後続の処理へ渡すために使います。
    return m_currentSlot == EWeaponSlot::AR && IsValid(m_pARWeapon) && !m_bIsDead && !m_bIsHealing && !m_bIsReloadingAnim && !m_bIsSwitchingWeapon;
}

//PersistentARの照準が利用できる状態を保証します。
void APlayerChara::EnsurePersistentRifleAim()
{
    //「!ShouldUsePersistentRifleAim()」が成立するとき、続けて「GetRifleAimState() == ERifleAimState::Ready && !IsRifleReadyToFire()」を判定します。
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
    //「m_currentSlot != EWeaponSlot::AR」が成立するとき、m_bRifleCombatAimを更新します。
    if (m_currentSlot != EWeaponSlot::AR) { return; }

    m_bRifleCombatAim = true;
    GetWorldTimerManager().ClearTimer(m_rifleCombatAimTimer);
    SwapCrosshairWidget();
}

//射撃終了後にライフルの構えを解除します。
void APlayerChara::EndRifleCombatAim()
{
    //「ShouldUsePersistentRifleAim()」が成立するとき、FinishRifleAttackを呼び出します。
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
    //「m_pRifleAnimationComponent」が成立するとき、StopAimSequenceを呼び出します。
    if (m_pRifleAnimationComponent)
    {
        m_pRifleAnimationComponent->StopAimSequence(_blendOutTime);
    }
}

//ReloadMontageEndedの通知を受けてゲーム状態へ反映します。
void APlayerChara::HandleReloadMontageEnded(UAnimMontage* _montage, bool _bInterrupted)
{
    (void)_montage;

    //「_bInterrupted」が成立するとき、続けて「AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon)」を判定します。
    if (_bInterrupted)
    {
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
        StopRifleAimPose(0.0f);
        return;
    }

    FinishReloadFromAnimation();
}

//アニメーションの通知を受け、リロードを完了します。
void APlayerChara::FinishReloadFromAnimation()
{
    //「!m_bIsReloadingAnim」が成立するとき、続けて「AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon)」を判定します。
    if (!m_bIsReloadingAnim) { return; }

    //「AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon)」が成立するとき、FinishReloadを呼び出します。
    if (AGunWeapon* gunWeapon = Cast<AGunWeapon>(m_pCurrentWeapon))
    {
        gunWeapon->FinishReload();
    }

    //「m_pAudioComponent」が成立するとき、StopReloadSoundを呼び出します。
    if (m_pAudioComponent)
    {
        m_pAudioComponent->StopReloadSound();
    }

    m_bIsReloadingAnim = false;
    StopRifleAimPose(0.0f);
}

//着地処理

void APlayerChara::AttachWeapon(TSubclassOf<AActor> WeaponClass)
{
    //「!WeaponClass」が成立するとき、続けて「EquippedWeapon」を判定します。
    if (!WeaponClass) { return; }
    //「EquippedWeapon」が成立するとき、Destroyを呼び出します。
    if (EquippedWeapon)
    {
        EquippedWeapon->Destroy();
        EquippedWeapon = nullptr;
    }
    //spは、直後の初期化結果を、同じスコープ内でこの名前を参照する計算や関数呼び出しへ渡すために使います。
    FActorSpawnParameters sp;
    sp.Owner = this;
    sp.Instigator = GetInstigator();
    //ワールドを返します。
    AWeaponBase* w = GetWorld()->SpawnActor<AWeaponBase>(WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, sp);
    //「!w」が成立するとき、SetOwnerCharacterを呼び出します。
    if (!w) { return; }
    w->SetOwnerCharacter(this);
    w->SetOwner(this);
    //「auto* mesh = GetMesh()」が成立するとき、rを呼び出します。
    if (auto* mesh = GetMesh())
    {
        //rは、rの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
        FAttachmentTransformRules r(EAttachmentRule::KeepRelative, true);
        w->AttachToComponent(mesh, r, WeaponSocketName);
    }
    EquippedWeapon = w;
    m_pCurrentWeapon = w;
}

//カメラ回転処理

void APlayerChara::StartAim()
{
    //「m_bIsAiming || m_bIsDead || m_bIsHealing || m_bIsReloadingAnim || m_bIsSwitchingWeapon」が成立するとき、ResetIdleTimerを呼び出します。
    if (m_bIsAiming || m_bIsDead || m_bIsHealing || m_bIsReloadingAnim || m_bIsSwitchingWeapon) { return; }
    ResetIdleTimer();
    m_bIsAiming = true;
    GetWorldTimerManager().ClearTimer(m_rifleCombatAimTimer);
    //「m_currentSlot == EWeaponSlot::AR && !m_bRifleCombatAim && !m_bIsFiringRifle」が成立するとき、BeginRifleAimTransitionを呼び出します。
    if (m_currentSlot == EWeaponSlot::AR && !m_bRifleCombatAim && !m_bIsFiringRifle)
    {
        BeginRifleAimTransition();
    }
    SwapCrosshairWidget();
}

//エイム終了

void APlayerChara::StopAim()
{
    //「!m_bIsAiming」が成立するとき、m_bIsAimingを更新します。
    if (!m_bIsAiming) { return; }
    m_bIsAiming = false;
    //「m_currentSlot == EWeaponSlot::AR」が成立するとき、続けて「!m_bRifleTriggerHeld」を判定します。
    if (m_currentSlot == EWeaponSlot::AR)
    {
        //「!m_bRifleTriggerHeld」が成立するとき、FinishRifleAttackを呼び出します。
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
