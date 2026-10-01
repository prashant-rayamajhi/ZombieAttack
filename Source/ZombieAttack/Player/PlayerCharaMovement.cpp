#include "PlayerChara.h"
#include "../Components/PlayerAudio/PlayerAudioComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"

namespace
{
//TryGetTorsoForwardは、必要条件を検査して実行可能な場合だけ名前が示す動作を開始します。
bool TryGetTorsoForward(const USkeletalMeshComponent* _mesh, const FVector& _desiredDirection, FVector& _outTorsoForward)
{
    if (!_mesh || _mesh->GetBoneIndex(TEXT("LeftShoulder")) == INDEX_NONE || _mesh->GetBoneIndex(TEXT("RightShoulder")) == INDEX_NONE ||
        _mesh->GetBoneIndex(TEXT("Hips")) == INDEX_NONE || _mesh->GetBoneIndex(TEXT("Head")) == INDEX_NONE)
    {
        //呼び出し元へ失敗を返し、この関数でこれ以上の処理を行わないようにします。
        return false;
    }

    //左右の肩を結ぶ方向を求めます。
    const FVector shoulderRight = (_mesh->GetBoneLocation(TEXT("RightShoulder")) - _mesh->GetBoneLocation(TEXT("LeftShoulder"))).GetSafeNormal();
    //腰から頭へ向かう上方向を求めます。
    const FVector torsoUp = (_mesh->GetBoneLocation(TEXT("Head")) - _mesh->GetBoneLocation(TEXT("Hips"))).GetSafeNormal();
    //肩と背骨の向きから上半身の正面を求めます。
    FVector torsoForward = FVector::CrossProduct(shoulderRight, torsoUp).GetSafeNormal();
    if (torsoForward.IsNearlyZero()) { return false; }
    if (FVector::DotProduct(torsoForward, _desiredDirection) < 0.0f)
    {
        torsoForward *= -1.0f;
    }

    _outTorsoForward = torsoForward;
    //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
    return true;
}
//名前空間を閉じます。
//名前空間を閉じます。
}

//アニメーション再生中の移動を止めます。
void APlayerChara::LockMovementByAnimation()
{
    m_bMovementLockedByAnimation = true;
    m_charaMovement = FVector2D::ZeroVector;
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }
}

//アニメーション再生後の移動を戻します。
void APlayerChara::UnlockMovementByAnimation() { m_bMovementLockedByAnimation = false; }

//AcceptMove入力を実行できる状態か判定します。
bool APlayerChara::CanAcceptMoveInput() const { return m_bCanControl && !m_bIsDead && !m_bMovementLockedByAnimation; }

//プレイヤー操作からの攻撃。アクション競合を一か所で防ぐ。

void APlayerChara::UpdateMove(float _deltaTime)
{
    if (!m_bCanControl || m_bMovementLockedByAnimation) { return; }

    //現在の状態に合わせた移動倍率を用意します。
    float scale = 1.0f;
    if (m_bIsReloadingAnim) scale = m_reloadSpeedScale;

    //後退歩行のARを前進走行と同じ速さで滑らせず、装備ごとの足運びに合う上限へ切り替える。
    const bool backward = m_charaMovement.Y < -0.05f;
    const float backwardSpeed = m_currentSlot == EWeaponSlot::AR ? 180.0f : 280.0f;
    GetCharacterMovement()->MaxWalkSpeed = backward ? FMath::Min(m_moveSpeed, backwardSpeed) : m_moveSpeed;

    //カメラのワールドYaw を取得
    float camYaw = m_pSpringArm->GetComponentRotation().Yaw;
    FRotator camRotYaw(0.f, camYaw, 0.f);

    //カメラ相対の前後・左右ベクトル
    const FVector cameraForward = FRotationMatrix(camRotYaw).GetUnitAxis(EAxis::X);
    //カメラから見た右方向を求めます。
    const FVector right = FRotationMatrix(camRotYaw).GetUnitAxis(EAxis::Y);
    //後退中も視点を維持し、入力方向だけをカメラの前後左右へ変換する。
    AddMovementInput(cameraForward, m_charaMovement.Y * scale);
    AddMovementInput(right, m_charaMovement.X * scale);

    //メッシュを返します。
    USkeletalMeshComponent* mesh = GetMesh();
    //ARの通常移動は既存のLocomotionを維持し、発砲可能になった射撃中だけ中央へ向けます。
    //Down To Aim中は弾が出ないため、Aiming Idleへ遷移してから姿勢を補正します。
    const bool bRifleAttackFacing = m_currentSlot == EWeaponSlot::AR && m_bRifleTriggerHeld && IsRifleReadyToFire();

    //Actorを回すとSpringArmまで動くため、射撃中は見た目を担うMeshだけを補正します。
    if (bRifleAttackFacing && mesh)
    {
        const float cameraRelativeYaw = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, camYaw);
        //肩越しカメラの横位置を補正し、銃口を画面中央側へ向けます。
        FRotator targetMeshRotation(m_defaultMeshRelativeRotation.Pitch,
                                    m_defaultMeshRelativeRotation.Yaw + cameraRelativeYaw + m_rifleAimInwardYawOffset,
                                    m_defaultMeshRelativeRotation.Roll);
        if (m_pCamera)
        {
            //カメラの中心線から上半身が向く目標点を決める
            const FVector aimCameraForward = m_pCamera->GetForwardVector().GetSafeNormal();
            const FVector aimPoint = m_pCamera->GetComponentLocation() + aimCameraForward * m_rifleVisualConvergenceDistance;
            const FVector spineLocation =
                mesh->GetBoneIndex(TEXT("Spine2")) != INDEX_NONE ? mesh->GetBoneLocation(TEXT("Spine2")) : GetActorLocation();
            const FVector desiredTorsoForward = (aimPoint - spineLocation).GetSafeNormal2D();
            FVector currentTorsoForward = FVector::ZeroVector;
            if (!desiredTorsoForward.IsNearlyZero() && TryGetTorsoForward(mesh, desiredTorsoForward, currentTorsoForward))
            {
                //現在のアニメーションが持つ胴体のねじれをローカル空間で取り出します。
                const FVector localTorsoForward = mesh->GetComponentTransform().InverseTransformVectorNoScale(currentTorsoForward).GetSafeNormal2D();
                if (!localTorsoForward.IsNearlyZero())
                {
                    const float desiredMeshWorldYaw =
                        desiredTorsoForward.Rotation().Yaw - localTorsoForward.Rotation().Yaw + m_rifleAimInwardYawOffset;
                    targetMeshRotation.Yaw = FRotator::NormalizeAxis(desiredMeshWorldYaw - GetActorRotation().Yaw);
                }
            }
        }

        //ARは弾道と見た目の向きが一瞬でも分離しないよう、同じフレームで中央へ合わせます。
        mesh->SetRelativeRotation(targetMeshRotation);
    }

    //キャラクターをカメラ方向に向ける
    if (!bRifleAttackFacing && !m_charaMovement.IsNearlyZero(0.05f))
    {
        //メッシュコンポーネントを取得
        //念のためメッシュがあるか確認
        if (!mesh) { return; }
        float newYaw = 0.0f;
        if (m_charaMovement.Y < -0.05f)
        {
            //S入力と斜め後退では敵のいる画面正面を向いたまま、後ろへ足を運ぶ。
            newYaw = GetBaseRotationOffsetRotator().Yaw + m_pSpringArm->GetRelativeRotation().Yaw;
        }
        else
        {
            //入力ベクトルの角度を計算
            const float angle = atan2(m_charaMovement.X, m_charaMovement.Y);

            //atan2の結果はラジアンなので、度に変換
            const float angleDeg = FMath::RadiansToDegrees(angle);

            //カメラの回転を考慮して、キャラクターの向きを計算
            newYaw = angleDeg + GetBaseRotationOffsetRotator().Yaw + m_pSpringArm->GetRelativeRotation().Yaw;
        }

        //メッシュの相対回転を更新（カメラ方向に向ける）
        mesh->SetRelativeRotation(FRotator(0.0f, newYaw, 0.0f));
        if (m_bIsAiming)
        {
            FRotator targetRot(0.f, camYaw, 0.f);
            FRotator currentRot = GetActorRotation();
            //武器切り替え中は回転を遅らせる（足滑り防止）
            float rotSpeed = m_bIsSwitchingWeapon ? 5.f : 12.f;
            FRotator newRot = FMath::RInterpTo(currentRot, targetRot, _deltaTime, rotSpeed);
            SetActorRotation(FRotator(0.f, newRot.Yaw, 0.f));
        }
    }
}

//ストレーフアニメ用: キャラ前方成分

void APlayerChara::AlignRifleVisualFacing()
{
    //メッシュを返します。
    USkeletalMeshComponent* playerMesh = GetMesh();
    if (m_currentSlot != EWeaponSlot::AR || !playerMesh || !m_pCamera) { return; }
    const FVector cameraForward = m_pCamera->GetForwardVector().GetSafeNormal();
    const FVector aimPoint = m_pCamera->GetComponentLocation() + cameraForward * m_rifleVisualConvergenceDistance;
    const FVector spineLocation =
        playerMesh->GetBoneIndex(TEXT("Spine2")) != INDEX_NONE ? playerMesh->GetBoneLocation(TEXT("Spine2")) : GetActorLocation();
    const FVector desiredTorsoForward = (aimPoint - spineLocation).GetSafeNormal2D();
    FVector currentTorsoForward = FVector::ZeroVector;
    if (desiredTorsoForward.IsNearlyZero() || !TryGetTorsoForward(playerMesh, desiredTorsoForward, currentTorsoForward)) { return; }
    const FVector localTorsoForward = playerMesh->GetComponentTransform().InverseTransformVectorNoScale(currentTorsoForward).GetSafeNormal2D();
    if (localTorsoForward.IsNearlyZero()) { return; }
    const float desiredMeshWorldYaw = desiredTorsoForward.Rotation().Yaw - localTorsoForward.Rotation().Yaw + m_rifleAimInwardYawOffset;
    const float desiredMeshRelativeYaw = FRotator::NormalizeAxis(desiredMeshWorldYaw - GetActorRotation().Yaw);

    playerMesh->SetRelativeRotation(FRotator(m_defaultMeshRelativeRotation.Pitch, desiredMeshRelativeYaw, m_defaultMeshRelativeRotation.Roll));
}

//VelocityForwardを取得して呼び出し元へ返します。
float APlayerChara::GetVelocityForward() const
{
    //速度を返します。
    const FVector velocity = GetVelocity();
    if (velocity.IsNearlyZero()) { return 0.f; }
    //Actorと独立して回る見た目の正面を基準にし、カメラを回した後も後退を正しく判定する。
    const float facingYaw = GetMesh()->GetComponentRotation().Yaw - GetBaseRotationOffsetRotator().Yaw;
    return FVector::DotProduct(velocity.GetSafeNormal2D(), FRotator(0.0f, facingYaw, 0.0f).Vector());
}

//ストレーフアニメ用

float APlayerChara::GetVelocityRight() const
{
    //速度を返します。
    const FVector velocity = GetVelocity();
    if (velocity.IsNearlyZero()) { return 0.f; }
    //後退と同じ基準で左右成分を求め、斜め移動時の足の向きを揃える。
    const float facingYaw = GetMesh()->GetComponentRotation().Yaw - GetBaseRotationOffsetRotator().Yaw;
    return FVector::DotProduct(velocity.GetSafeNormal2D(), FRotator(0.0f, facingYaw + 90.0f, 0.0f).Vector());
}

//アイドル状態管理

void APlayerChara::UpdateIdleState(float _deltaTime)
{
    //プレイヤーが停止判定を超える速度で移動しているかを示します。
    bool bMoving = !m_charaMovement.IsNearlyZero(0.01f);
    if (bMoving)
    {
        m_idleState = EIdleState::GameplayIdle;
        m_idleTimerAccum = 0.f;
    }
    else
    {
        m_idleTimerAccum += _deltaTime;
        if (m_idleTimerAccum >= m_idleTimeoutSeconds)
        {
            m_idleState = EIdleState::InitialIdle;
        }
    }
}

//アイドル状態リセット（攻撃や回復などのアクション開始時に呼び出す）

void APlayerChara::ResetIdleTimer()
{
    m_idleState = EIdleState::GameplayIdle;
    m_idleTimerAccum = 0.f;
}

//ダメージ処理

void APlayerChara::Landed(const FHitResult& _hit)
{
    Super::Landed(_hit);
    if (m_pAudioComponent)
    {
        m_pAudioComponent->PlayLanding();
    }

    EndJump();

    //着地アニメ中に足滑りしないように移動入力を止めます。
    LockMovementByAnimation();
    float UnlockDelay = m_landingFallbackUnlockTime;
    if (m_pLandingMontage)
    {
        const float MontageLength = PlayAnimMontage(m_pLandingMontage);
        if (MontageLength > 0.f)
        {
            UnlockDelay = MontageLength;
        }
    }

    GetWorldTimerManager().ClearTimer(m_movementLockTimer);
    GetWorldTimerManager().SetTimer(m_movementLockTimer, this, &APlayerChara::UnlockMovementByAnimation, FMath::Max(0.01f, UnlockDelay), false);
}

//ジャンプアニメ用の情報を返す関数（AnimNotifyから呼び出す）

bool APlayerChara::GetJumpAnimInfo(bool _bJumpUp)
{
    if (!m_bJumping) { return false; }
    return _bJumpUp ? GetVelocity().Z > 0.f : GetVelocity().Z <= 0.f;
}

//エイムのピッチ角度を取得（AnimNotifyから呼び出す）

float APlayerChara::GetAimPitch() const
{
    //コントローラーを返します。
    AController* ctrl = GetController();
    return ctrl ? ctrl->GetControlRotation().Pitch : 0.f;
}

//武器の装備（AnimNotifyから呼び出す）

void APlayerChara::UpdateCamera(float _deltaTime)
{
    //カメラ回転の更新
    //フレームレートに依存しないカメラ回転です。_deltaTimeが極端に小さい場合も安全にします。
    const float rotateCorrection = 60.f * FMath::Max(0.f, _deltaTime);

    //現在の角度を取得
    FRotator newRot = m_pSpringArm->GetRelativeRotation();

    //入力値を加算
    newRot.Yaw += m_cameraRotation.X * rotateCorrection;

    //ピッチは制限内にクランプ
    newRot.Pitch = FMath::Clamp(newRot.Pitch + (m_cameraRotation.Y * rotateCorrection), m_cameraPitchLimit.X, m_cameraPitchLimit.Y);

    //スプリングアームの回転を更新
    m_pSpringArm->SetRelativeRotation(newRot);
}

//カメラRelative回転を取得して呼び出し元へ返します。
FRotator APlayerChara::GetCameraRelativeRotation() const { return m_pSpringArm ? m_pSpringArm->GetRelativeRotation() : FRotator::ZeroRotator; }

//RifleFacing画面Centerかを判定します。
bool APlayerChara::IsRifleFacingScreenCenter(float _toleranceDegrees) const
{
    //メッシュを返します。
    const USkeletalMeshComponent* playerMesh = GetMesh();
    if (m_currentSlot != EWeaponSlot::AR || !playerMesh || !m_pCamera) { return false; }
    const FVector cameraForward = m_pCamera->GetForwardVector().GetSafeNormal();
    const FVector aimPoint = m_pCamera->GetComponentLocation() + cameraForward * m_rifleVisualConvergenceDistance;
    const FVector spineLocation =
        playerMesh->GetBoneIndex(TEXT("Spine2")) != INDEX_NONE ? playerMesh->GetBoneLocation(TEXT("Spine2")) : GetActorLocation();
    const FVector desiredTorsoForward = (aimPoint - spineLocation).GetSafeNormal2D();
    //肩と背骨の向きから上半身の正面を求めます。
    FVector torsoForward = FVector::ZeroVector;
    if (!TryGetTorsoForward(playerMesh, desiredTorsoForward, torsoForward)) { return false; }

    const float torsoYawError =
        //入力値の大きさを符号なしで求めます。
        FMath::Abs(FMath::FindDeltaAngleDegrees(torsoForward.Rotation().Yaw, desiredTorsoForward.Rotation().Yaw));
    return torsoYawError <= FMath::Max(_toleranceDegrees, 0.0f);
}

//ジャンプ処理

void APlayerChara::UpdateJump(float _deltaTime)
{
    //キャラクター移動を返します。
    const UCharacterMovementComponent* Movement = GetCharacterMovement();
    m_bJumping = Movement && Movement->IsFalling();
}

//ジャンプ終了処理

void APlayerChara::EndJump()
{
    StopJumping();
    m_bJumping = false;
}

//クロスヘアの位置更新

void APlayerChara::Cam_RotatePitch(float _value) { m_cameraRotation.Y = _value; }

//横方向の入力をControllerへ加え、カメラを左右へ回転させます。
void APlayerChara::Cam_RotateYaw(float _value) { m_cameraRotation.X = _value; }

//移動入力を受け取る関数

void APlayerChara::Chara_MoveForward(float _value)
{
    if (!CanAcceptMoveInput())
    {
        m_charaMovement.Y = 0.f;
        return;
    }
    m_charaMovement.Y = FMath::Clamp(_value, -1.f, 1.f);
    if (!FMath::IsNearlyZero(_value)) ResetIdleTimer();
}

//カメラ基準の右方向へプレイヤーの移動入力を加えます。
void APlayerChara::Chara_MoveRight(float _value)
{
    if (!CanAcceptMoveInput())
    {
        m_charaMovement.X = 0.f;
        return;
    }
    m_charaMovement.X = FMath::Clamp(_value, -1.f, 1.f);
    if (!FMath::IsNearlyZero(_value))
    {
        ResetIdleTimer();
    }
}

//ジャンプ開始

void APlayerChara::JumpStart()
{
    if (!m_bCanControl || m_bIsDead || !CanJump()) { return; }
    ResetIdleTimer();
    if (m_jumpStartMoveLockTime > 0.f)
    {
        LockMovementByAnimation();
        GetWorldTimerManager().ClearTimer(m_movementLockTimer);
        GetWorldTimerManager().SetTimer(m_movementLockTimer, this, &APlayerChara::UnlockMovementByAnimation, m_jumpStartMoveLockTime, false);
    }

    Jump();
    m_bJumping = true;
}

//攻撃
