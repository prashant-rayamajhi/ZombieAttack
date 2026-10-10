#include "PlayerChara.h"
#include "../Components/PlayerAudio/PlayerAudioComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "../Components/RifleAnimation/PlayerRifleAnimationComponent.h"

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
    //構え始めからカメラへ向ける。肩の揺れを回転へ戻すと毎フレーム補正が振動するため使わない。
    const bool bRifleAttackFacing = m_currentSlot == EWeaponSlot::AR && (m_bRifleTriggerHeld || m_bIsAiming || IsRifleReadyToFire());
    if ((bRifleAttackFacing || m_bIsAiming) && mesh)
    {
        if (bRifleAttackFacing)
        {
            AlignRifleVisualFacing();
        }
        else
        {
            //カメラを持つ本体は回さず、見た目だけを照準の正面へ合わせる。
            const float yaw = camYaw - GetActorRotation().Yaw + m_defaultMeshRelativeRotation.Yaw;
            mesh->SetRelativeRotation(FRotator(m_defaultMeshRelativeRotation.Pitch, yaw, m_defaultMeshRelativeRotation.Roll));
        }
    }

    //キャラクターをカメラ方向に向ける
    if (!bRifleAttackFacing && !m_bIsAiming && !m_charaMovement.IsNearlyZero(0.05f))
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
    }
}

//ストレーフアニメ用: キャラ前方成分

void APlayerChara::AlignRifleVisualFacing()
{
    USkeletalMeshComponent* mesh = GetMesh();
    if (m_currentSlot != EWeaponSlot::AR || !mesh || !m_pCamera) { return; }
    //構えクリップ内の両手の向きを画面正面へ合わせ、武器BPの取り付け角度には触れない。
    const FVector direction = m_pRifleAnimationComponent ? m_pRifleAnimationComponent->GetAimPoseDirection() : FVector::RightVector;
    const float yaw = m_pCamera->GetComponentRotation().Yaw - GetActorRotation().Yaw - direction.Rotation().Yaw;
    mesh->SetRelativeRotation(FRotator(m_defaultMeshRelativeRotation.Pitch, FRotator::NormalizeAxis(yaw), m_defaultMeshRelativeRotation.Roll));
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
    const USkeletalMeshComponent* mesh = GetMesh();
    if (m_currentSlot != EWeaponSlot::AR || !mesh || !m_pCamera) { return false; }
    //構えクリップの射撃方向とカメラの水平角を比べ、反動の揺れで発砲を止めない。
    const FVector direction = m_pRifleAnimationComponent ? m_pRifleAnimationComponent->GetAimPoseDirection() : FVector::RightVector;
    const float facingYaw = mesh->GetComponentRotation().Yaw + direction.Rotation().Yaw;
    return FMath::Abs(FMath::FindDeltaAngleDegrees(facingYaw, m_pCamera->GetComponentRotation().Yaw)) <= FMath::Max(0.0f, _toleranceDegrees);
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
