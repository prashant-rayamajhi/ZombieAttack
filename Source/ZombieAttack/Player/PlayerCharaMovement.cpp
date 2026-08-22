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
    //「!_mesh || _mesh->GetBoneIndex(TEXT("LeftShoulder")」が成立するとき、GetBoneIndexを呼び出します。
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

    //「torsoForward.IsNearlyZero()」が成立するとき、続けて「FVector::DotProduct(torsoForward, _desiredDirection) < 0.0f」を判定します。
    if (torsoForward.IsNearlyZero()) { return false; }
    //「FVector::DotProduct(torsoForward, _desiredDirection) < 0.0f」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
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

    //「UCharacterMovementComponent* Movement = GetCharacterMovement()」が成立するとき、StopMovementImmediatelyを呼び出します。
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
    //「!m_bCanControl || m_bMovementLockedByAnimation」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (!m_bCanControl || m_bMovementLockedByAnimation) { return; }

    //現在の状態に合わせた移動倍率を用意します。
    float scale = 1.0f;
    //「m_bIsReloadingAnim」が成立するとき、GetComponentRotationを呼び出します。
    if (m_bIsReloadingAnim) scale = m_reloadSpeedScale;

    //カメラのワールドYaw を取得
    float camYaw = m_pSpringArm->GetComponentRotation().Yaw;
    //camRotYawは、camRotYawの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    FRotator camRotYaw(0.f, camYaw, 0.f);

    //カメラ相対の前後・左右ベクトル
    const FVector fwd = FRotationMatrix(camRotYaw).GetUnitAxis(EAxis::X);
    //カメラから見た右方向を求めます。
    const FVector right = FRotationMatrix(camRotYaw).GetUnitAxis(EAxis::Y);
    //入力からワールド上の移動方向を求めます。
    const FVector requestedMoveDirection = (fwd * m_charaMovement.Y + right * m_charaMovement.X).GetSafeNormal2D();

    //Pitch操作や僅かなスティックドリフトでは解除せず、明確なYaw操作だけを優先します。
    const bool bHasManualCameraInput = FMath::Abs(m_cameraRotation.X) >= 0.12f;
    //「bHasManualCameraInput」が成立するとき、m_cameraManualOverrideRemainingを更新します。
    if (bHasManualCameraInput)
    {
        //視点操作は常にプレイヤーを優先し、自動追従をすぐ解除します。
        m_cameraManualOverrideRemaining = m_cameraManualOverrideDuration;
        b_mBackwardCameraFollowActive = false;
        m_backwardInputHoldTime = 0.0f;
        m_backwardMoveDirection = FVector::ZeroVector;
    }
    else
    {
        m_cameraManualOverrideRemaining = FMath::Max(0.0f, m_cameraManualOverrideRemaining - _deltaTime);
    }

    //これにより斜め入力、ゲームパッドのドリフト、キャラクターの向きに影響されません。
    const float cameraMovementAlignment = requestedMoveDirection.IsNearlyZero() ? 1.0f : FVector::DotProduct(fwd, requestedMoveDirection);
    //カメラの逆方向へ移動しているか確認します。
    const bool bIsMovingAgainstCamera = cameraMovementAlignment <= -m_backwardCameraFollowThreshold;
    //視点の自動追従を使える状態か確認します。
    const bool bCanUseCameraAssist = bIsMovingAgainstCamera && m_cameraManualOverrideRemaining <= 0.0f && !m_bIsAiming && !m_bRifleTriggerHeld;

    //「bCanUseCameraAssist」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (bCanUseCameraAssist)
    {
        m_backwardInputHoldTime += _deltaTime;

        //「!b_mBackwardCameraFollowActive && m_backwardInputHoldTime >= m_backwardCameraFollowDelay」が成立するとき、m_backwardMoveDirectionを更新します。
        if (!b_mBackwardCameraFollowActive && m_backwardInputHoldTime >= m_backwardCameraFollowDelay)
        {
            m_backwardMoveDirection = requestedMoveDirection;
            m_backwardCameraTargetWorldYaw = m_backwardMoveDirection.Rotation().Yaw;
            b_mBackwardCameraFollowActive = !m_backwardMoveDirection.IsNearlyZero();
        }
    }
    else
    {
        b_mBackwardCameraFollowActive = false;
        m_backwardInputHoldTime = 0.0f;
        m_backwardMoveDirection = FVector::ZeroVector;
    }

    //「b_mBackwardCameraFollowActive」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (b_mBackwardCameraFollowActive)
    {
        //一定速度へ徐々に加速させることで、急な引っ張り感や酔いやすさを抑えます。
        const float followElapsedTime = m_backwardInputHoldTime - m_backwardCameraFollowDelay;
        //自動追従の加速率を0から1に収めます。
        const float accelerationAlpha =
            FMath::Clamp(followElapsedTime / FMath::Max(m_backwardCameraFollowAccelerationTime, UE_SMALL_NUMBER), 0.0f, 1.0f);
        //視点の動きが急に変わらないよう加速率を整えます。
        const float easedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, accelerationAlpha, 2.0f);
        //このフレームで回せる最大角を求めます。
        const float maximumYawStep = m_backwardCameraFollowYawSpeed * FMath::Max(easedAlpha, 0.08f) * _deltaTime;

        //スプリングアームの現在の回転を取得します。
        FRotator springArmRotation = m_pSpringArm->GetRelativeRotation();
        //コンポーネント回転を返します。
        const float currentWorldYaw = m_pSpringArm->GetComponentRotation().Yaw;
        //このフレームで使う左右角を求めます。
        const float nextWorldYaw = FMath::FixedTurn(currentWorldYaw, m_backwardCameraTargetWorldYaw, maximumYawStep);
        springArmRotation.Yaw = FRotator::NormalizeAxis(nextWorldYaw - GetActorRotation().Yaw);
        m_pSpringArm->SetRelativeRotation(springArmRotation);

        //移動入力の強さを0から1に収めます。
        const float inputMagnitude = FMath::Clamp(m_charaMovement.Size(), 0.0f, 1.0f);
        AddMovementInput(m_backwardMoveDirection, inputMagnitude * scale);
    }
    else
    {
        AddMovementInput(fwd, m_charaMovement.Y * scale);
        AddMovementInput(right, m_charaMovement.X * scale);
    }

    //メッシュを返します。
    USkeletalMeshComponent* mesh = GetMesh();
    //ARの通常移動は既存のLocomotionを維持し、発砲可能になった射撃中だけ中央へ向けます。
    //Down To Aim中は弾が出ないため、Aiming Idleへ遷移してから姿勢を補正します。
    const bool bRifleAttackFacing = m_currentSlot == EWeaponSlot::AR && m_bRifleTriggerHeld && IsRifleReadyToFire();

    //Actorを回すとSpringArmまで動くため、射撃中は見た目を担うMeshだけを補正します。
    if (bRifleAttackFacing && mesh)
    {
        //cameraRelativeYawは、FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, camYaw)から算出した数値を後続の判定または計算に使います。
        const float cameraRelativeYaw = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, camYaw);
        //肩越しカメラの横位置を補正し、銃口を画面中央側へ向けます。
        FRotator targetMeshRotation(m_defaultMeshRelativeRotation.Pitch,
                                    m_defaultMeshRelativeRotation.Yaw + cameraRelativeYaw + m_rifleAimInwardYawOffset,
                                    m_defaultMeshRelativeRotation.Roll);

        //「m_pCamera」が成立するとき、GetForwardVectorを呼び出します。
        if (m_pCamera)
        {
            //cameraForwardは、m_pCamera->GetForwardVector().GetSafeNormal()から求めた空間情報を位置または向きの計算に使います。
            const FVector cameraForward = m_pCamera->GetForwardVector().GetSafeNormal();
            //コンポーネント位置を返します。
            const FVector aimPoint = m_pCamera->GetComponentLocation() + cameraForward * m_rifleVisualConvergenceDistance;
            //spineLocationは、位置と向きの計算結果を移動、照準、または描画位置へ反映するために使います。
            const FVector spineLocation =
                mesh->GetBoneIndex(TEXT("Spine2")) != INDEX_NONE ? mesh->GetBoneLocation(TEXT("Spine2")) : GetActorLocation();
            //desiredTorsoForwardは、(aimPoint - spineLocation).GetSafeNormal2D()から求めた空間情報を位置または向きの計算に使います。
            const FVector desiredTorsoForward = (aimPoint - spineLocation).GetSafeNormal2D();
            //currentTorsoForwardは、FVector::ZeroVectorから求めた空間情報を位置または向きの計算に使います。
            FVector currentTorsoForward = FVector::ZeroVector;

            //「!desiredTorsoForward.IsNearlyZero(」が成立するとき、TryGetTorsoForwardを呼び出します。
            if (!desiredTorsoForward.IsNearlyZero() && TryGetTorsoForward(mesh, desiredTorsoForward, currentTorsoForward))
            {
                //現在のアニメーションが持つ胴体のねじれをローカル空間で取り出します。
                const FVector localTorsoForward = mesh->GetComponentTransform().InverseTransformVectorNoScale(currentTorsoForward).GetSafeNormal2D();
                //「!localTorsoForward.IsNearlyZero()」が成立するとき、Rotationを呼び出します。
                if (!localTorsoForward.IsNearlyZero())
                {
                    //desiredMeshWorldYawは、対象のゲームオブジェクトへ安全にアクセスするために使います。
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

        //newYawは、0.0fから算出した数値を後続の判定または計算に使います。
        float newYaw = 0.0f;
        //「b_mBackwardCameraFollowActive」が成立するとき、newYawを更新します。
        if (b_mBackwardCameraFollowActive)
        {
            //Cameraが追従している間も、Meshは固定した進行方向を向き続ける
            newYaw = FRotator::NormalizeAxis(m_backwardCameraTargetWorldYaw - GetActorRotation().Yaw + GetBaseRotationOffsetRotator().Yaw);
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

        //「m_bIsAiming」が成立するとき、targetRotを呼び出します。
        if (m_bIsAiming)
        {
            //targetRotは、0.f, camYaw, 0.f)から求めた空間情報を位置または向きの計算に使います。
            FRotator targetRot(0.f, camYaw, 0.f);
            //currentRotは、GetActorRotation()から求めた空間情報を位置または向きの計算に使います。
            FRotator currentRot = GetActorRotation();
            //武器切り替え中は回転を遅らせる（足滑り防止）
            float rotSpeed = m_bIsSwitchingWeapon ? 5.f : 12.f;
            //newRotは、FMath::RInterpTo(currentRot, targetRot, _deltaTime, rotSpeed)から求めた空間情報を位置または向きの計算に使います。
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
    //「m_currentSlot != EWeaponSlot::AR || !playerMesh || !m_pCamera」が成立するとき、GetForwardVectorを呼び出します。
    if (m_currentSlot != EWeaponSlot::AR || !playerMesh || !m_pCamera) { return; }

    //cameraForwardは、m_pCamera->GetForwardVector().GetSafeNormal()から求めた空間情報を位置または向きの計算に使います。
    const FVector cameraForward = m_pCamera->GetForwardVector().GetSafeNormal();
    //aimPointは、m_pCamera->GetComponentLocation() + cameraForward * m_rifleVisualConver…から求めた空間情報を位置または向きの計算に使います。
    const FVector aimPoint = m_pCamera->GetComponentLocation() + cameraForward * m_rifleVisualConvergenceDistance;
    const FVector spineLocation =
        playerMesh->GetBoneIndex(TEXT("Spine2")) != INDEX_NONE ? playerMesh->GetBoneLocation(TEXT("Spine2")) : GetActorLocation();
    //desiredTorsoForwardは、(aimPoint - spineLocation).GetSafeNormal2D()から求めた空間情報を位置または向きの計算に使います。
    const FVector desiredTorsoForward = (aimPoint - spineLocation).GetSafeNormal2D();
    //currentTorsoForwardは、FVector::ZeroVectorから求めた空間情報を位置または向きの計算に使います。
    FVector currentTorsoForward = FVector::ZeroVector;

    //「desiredTorsoForward.IsNearlyZero(」が成立するとき、TryGetTorsoForwardを呼び出します。
    if (desiredTorsoForward.IsNearlyZero() || !TryGetTorsoForward(playerMesh, desiredTorsoForward, currentTorsoForward)) { return; }

    //localTorsoForwardは、playerMesh->GetComponentTransform().InverseTransformVectorNoScale(curre…から求めた空間情報を位置または向きの計算に使います。
    const FVector localTorsoForward = playerMesh->GetComponentTransform().InverseTransformVectorNoScale(currentTorsoForward).GetSafeNormal2D();
    //「localTorsoForward.IsNearlyZero()」が成立するとき、Rotationを呼び出します。
    if (localTorsoForward.IsNearlyZero()) { return; }

    //desiredMeshWorldYawは、desiredTorsoForward.Rotation().Yaw - localTorsoForward.Rotation().Yaw +…から算出した数値を後続の判定または計算に使います。
    const float desiredMeshWorldYaw = desiredTorsoForward.Rotation().Yaw - localTorsoForward.Rotation().Yaw + m_rifleAimInwardYawOffset;
    //desiredMeshRelativeYawは、FRotator::NormalizeAxis(desiredMeshWorldYaw - GetActorRotation().Yaw)から算出した数値を後続の判定または計算に使います。
    const float desiredMeshRelativeYaw = FRotator::NormalizeAxis(desiredMeshWorldYaw - GetActorRotation().Yaw);

    playerMesh->SetRelativeRotation(FRotator(m_defaultMeshRelativeRotation.Pitch, desiredMeshRelativeYaw, m_defaultMeshRelativeRotation.Roll));
}

//VelocityForwardを取得して呼び出し元へ返します。
float APlayerChara::GetVelocityForward() const
{
    //速度を返します。
    FVector vel = GetVelocity();
    //「vel.IsNearlyZero()」が成立するとき、この関数を終了します。
    if (vel.IsNearlyZero()) { return 0.f; }
    return FVector::DotProduct(vel.GetSafeNormal2D(), GetActorForwardVector());
}

//ストレーフアニメ用

float APlayerChara::GetVelocityRight() const
{
    //速度を返します。
    FVector vel = GetVelocity();
    //「vel.IsNearlyZero()」が成立するとき、この関数を終了します。
    if (vel.IsNearlyZero()) { return 0.f; }
    return FVector::DotProduct(vel.GetSafeNormal2D(), GetActorRightVector());
}

//アイドル状態管理

void APlayerChara::UpdateIdleState(float _deltaTime)
{
    //プレイヤーが停止判定を超える速度で移動しているかを示します。
    bool bMoving = !m_charaMovement.IsNearlyZero(0.01f);
    //「bMoving」が成立するとき、m_idleStateを更新します。
    if (bMoving)
    {
        m_idleState = EIdleState::GameplayIdle;
        m_idleTimerAccum = 0.f;
    }
    else
    {
        m_idleTimerAccum += _deltaTime;
        //「m_idleTimerAccum >= m_idleTimeoutSeconds」が成立するとき、m_idleStateを更新します。
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

void APlayerChara::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);

    //「m_pAudioComponent」が成立するとき、PlayLandingを呼び出します。
    if (m_pAudioComponent)
    {
        m_pAudioComponent->PlayLanding();
    }

    EndJump();

    //着地アニメ中に足滑りしないように移動入力を止めます。
    LockMovementByAnimation();

    //UnlockDelayは、m_landingFallbackUnlockTimeから算出した数値を後続の判定または計算に使います。
    float UnlockDelay = m_landingFallbackUnlockTime;
    //「m_pLandingMontage」が成立するとき、PlayAnimMontageを呼び出します。
    if (m_pLandingMontage)
    {
        //MontageLengthは、PlayAnimMontage(m_pLandingMontage)から算出した数値を後続の判定または計算に使います。
        const float MontageLength = PlayAnimMontage(m_pLandingMontage);
        //「MontageLength > 0.f」が成立するとき、UnlockDelayを更新します。
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
    //「!m_bJumping」が成立するとき、この関数を終了します。
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
    //フレームレートに依存しないカメラ回転です。DeltaTimeが極端に小さい場合も安全にします。
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
    //「m_currentSlot != EWeaponSlot::AR || !playerMesh || !m_pCamera」が成立するとき、GetForwardVectorを呼び出します。
    if (m_currentSlot != EWeaponSlot::AR || !playerMesh || !m_pCamera) { return false; }

    //cameraForwardは、m_pCamera->GetForwardVector().GetSafeNormal()から求めた空間情報を位置または向きの計算に使います。
    const FVector cameraForward = m_pCamera->GetForwardVector().GetSafeNormal();
    //aimPointは、m_pCamera->GetComponentLocation() + cameraForward * m_rifleVisualConver…から求めた空間情報を位置または向きの計算に使います。
    const FVector aimPoint = m_pCamera->GetComponentLocation() + cameraForward * m_rifleVisualConvergenceDistance;
    const FVector spineLocation =
        playerMesh->GetBoneIndex(TEXT("Spine2")) != INDEX_NONE ? playerMesh->GetBoneLocation(TEXT("Spine2")) : GetActorLocation();
    //desiredTorsoForwardは、(aimPoint - spineLocation).GetSafeNormal2D()から求めた空間情報を位置または向きの計算に使います。
    const FVector desiredTorsoForward = (aimPoint - spineLocation).GetSafeNormal2D();
    //肩と背骨の向きから上半身の正面を求めます。
    FVector torsoForward = FVector::ZeroVector;
    //「!TryGetTorsoForward(playerMesh, desiredTorsoForward, torsoForward)」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
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
    //「!CanAcceptMoveInput()」が成立するとき、m_charaMovement.Yを更新します。
    if (!CanAcceptMoveInput())
    {
        m_charaMovement.Y = 0.f;
        return;
    }
    m_charaMovement.Y = FMath::Clamp(_value, -1.f, 1.f);
    //「!FMath::IsNearlyZero(_value)) ResetIdleTimer(」が成立するとき、Chara_MoveRightを呼び出します。
    if (!FMath::IsNearlyZero(_value)) ResetIdleTimer();
}

//カメラ基準の右方向へプレイヤーの移動入力を加えます。
void APlayerChara::Chara_MoveRight(float _value)
{
    //「!CanAcceptMoveInput()」が成立するとき、m_charaMovement.Xを更新します。
    if (!CanAcceptMoveInput())
    {
        m_charaMovement.X = 0.f;
        return;
    }
    m_charaMovement.X = FMath::Clamp(_value, -1.f, 1.f);
    //「!FMath::IsNearlyZero(_value)」が成立するとき、ResetIdleTimerを呼び出します。
    if (!FMath::IsNearlyZero(_value))
    {
        ResetIdleTimer();
    }
}

//ジャンプ開始

void APlayerChara::JumpStart()
{
    //「!m_bCanControl || m_bIsDead || !CanJump()」が成立するとき、ResetIdleTimerを呼び出します。
    if (!m_bCanControl || m_bIsDead || !CanJump()) { return; }
    ResetIdleTimer();

    //「m_jumpStartMoveLockTime > 0.f」が成立するとき、LockMovementByAnimationを呼び出します。
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
