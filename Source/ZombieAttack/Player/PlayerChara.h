#pragma once

#include "CoreMinimal.h"
#include "../Character/BaseCharacter.h"
#include "../Components/RifleAnimation/RifleAnimationTypes.h"
#include "../PickUp/PickUpBase.h"
#include "../Components/Combat/AttackInputBuffer.h"
#include "PlayerChara.generated.h"

//前方宣言
class USpringArmComponent;
//UCameraComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UCameraComponent;
//UPointLightComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UPointLightComponent;
//AMeleeWeaponは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class AMeleeWeapon;
//UAnimMontageは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAnimMontage;
//UAnimSequenceBaseは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAnimSequenceBase;
//AWeaponBaseは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class AWeaponBase;
//UUserWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UUserWidget;
//AGunWeaponは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class AGunWeapon;
//AARWeaponは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class AARWeapon;
//UWeaponCarouselWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UWeaponCarouselWidget;
//UEnemyLocatorWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UEnemyLocatorWidget;
//UPlayerAudioComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UPlayerAudioComponent;
//UPlayerRifleAnimationComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UPlayerRifleAnimationComponent;
//UAmmoHUDWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAmmoHUDWidget;
//UCombatCrosshairWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UCombatCrosshairWidget;

//武器スロットの列挙型
UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
    //ハンドガンを装備するスロット
    Pistol UMETA(DisplayName = "Pistol"),
    //ARを装備するスロット
    AR UMETA(DisplayName = "AR"),
    //ナイフを装備するスロット
    Knife UMETA(DisplayName = "Knife")
};

//待機アニメーション状態の列挙型
UENUM(BlueprintType)
enum class EIdleState : uint8
{
    //操作開始前の待機状態
    InitialIdle UMETA(DisplayName = "Initial Idle"),
    //ゲームプレイ中の待機状態
    GameplayIdle UMETA(DisplayName = "Gameplay Idle")
};

//プレイヤーキャラクターの動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API APlayerChara : public ABaseCharacter
{
    //エンジンが使う定型コード
    GENERATED_BODY()

public:
    //画面遷移でプレイヤーが消える場合も、撃破演出の時間倍率を戻す。
    virtual void EndPlay(const EEndPlayReason::Type _reason) override;
private:
    //行動終了直前に押された射撃を一回だけ受け付ける。
    FAttackInputBuffer m_attackBuffer;
    //行動制限と装備を照合し、期限内の射撃予約だけを実行する。
    void UpdateBufferedAttack();
    //連続撃破で画面停止を延長しないための実時間。
    double m_lastKillStop = -100.0;
    //演出前の時間倍率を保ち、他のスロー演出を強制解除しない。
    float m_beforeKillStop = 1.0f;
    //自分が開始したヒットストップだけを終了させる。
    bool m_killStopActive = false;

  public:
    //プレイヤーキャラクターを処理します。
    APlayerChara();

  protected:
    //ゲーム開始時の初期設定を行います。
    virtual void BeginPlay() override;

  public:
    //毎フレームの更新を行います。
    virtual void Tick(float _deltaTime) override;
    //入力コンポーネントの処理を担当するコンポーネント
    virtual void SetupPlayerInputComponent(class UInputComponent* _pInputComp) override;
    //装備中の武器で攻撃します。
    virtual void Attack() override;
    //着地後の状態を整えます。
    virtual void Landed(const FHitResult& _hit) override;

    //アニメーション再生中の移動を止めます。
    UFUNCTION(BlueprintCallable, Category = "Player|Movement Lock")
    void LockMovementByAnimation();

    //アニメーション再生後の移動を戻します。
    UFUNCTION(BlueprintCallable, Category = "Player|Movement Lock")
    void UnlockMovementByAnimation();

    //アニメーションにより移動が止められているかを返します。
    UFUNCTION(BlueprintCallable, Category = "Player|Movement Lock")
    bool IsMovementLockedByAnimation() const { return m_bMovementLockedByAnimation; }

    //最大体力を返します。
    float GetMaxHP() { return m_maxHp; }
    //現在装備している武器を返します。
    AWeaponBase* GetCurrentWeapon() const { return m_pCurrentWeapon; }

    //PlayKnifeAttackMontageは、名前が示す動作を開始するための初期状態を整えます。
    bool PlayKnifeAttackMontage(int32 _comboIndex);

    //ナイフ武器側が、プレイヤー側のMontage数を確認するために使います。
    int32 GetKnifeAttackMontageCount() const { return m_knifeAttackMontages.Num(); }

    //受けたダメージを体力へ反映します。
    virtual float TakeDamage(float _damageAmount, FDamageEvent const& _damageEvent, AController* _eventInstigator,
                             //overrideの操作に使用する参照です。
                             AActor* _damageCauser) override;

    //死亡MontageのNotifyと保険タイマーの両方から呼びます。
    //2回呼ばれてもGameOverへ二重遷移しないようにしています。
    UFUNCTION(BlueprintCallable, Category = "Death")
    void FinishDeathSequence();

    //アイテムを取得し、効果を反映します。
    bool PickUpItem(float _value, EItemType _type);
    //取得した回復量を体力へ反映します。
    void ApplyHeal(float _amount);
    //アニメーションの通知を受け、リロードを完了します。
    void FinishReloadFromAnimation();
    //アニメーションの通知を受け、ライフルのエイム移行を完了します。
    void FinishRifleAimTransitionFromAnimation();
    //敵を倒したことをゲーム側へ通知します。
    void NotifyEnemyDefeated();

    //射撃で敵を倒した瞬間だけ、短いヒットストップを再生します。
    void PlayKillHitStop();

    //カットシーンなどからクロスヘアだけを一時的に非表示にします。
    void SetCrosshairSuppressed(bool _bSuppressed);

  public:
    //AnimBPから参照する関数
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool GetJumpAnimInfo(bool _bJumpUp);

    //ジャンプ中かどうかを返します。
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool IsJumping() const { return m_bJumping; }

    //エイム中かどうかを返します。
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool IsAiming() const;

    //ライフルエイム状態を返します。
    UFUNCTION(BlueprintPure, Category = "Animation|Rifle")
    ERifleAimState GetRifleAimState() const;

    //エイム上下角を返します。
    UFUNCTION(BlueprintCallable, Category = "Animation")
    float GetAimPitch() const;

    //回復中かどうかを返します。
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool IsHealing() const { return m_bIsHealing; }

    //リロード中かどうかを返します。
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool IsReloading() const { return m_bIsReloadingAnim; }

    //武器の切り替え中かどうかを返します。
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool IsSwitchingWeapon() const { return m_bIsSwitchingWeapon; }
#if WITH_DEV_AUTOMATION_TESTS
    //持ち替え完了処理の検証だけに非公開メンバーへのアクセスを許可し、ゲーム用APIには追加しない。
    friend class FPlayerEquipmentTest;
#endif

    //死亡状態かどうかを返します。
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool IsDead() const { return m_bIsDead; }

    //現在のスロットを返します。
    UFUNCTION(BlueprintCallable, Category = "Animation")
    EWeaponSlot GetCurrentSlot() const { return m_currentSlot; }

    //GetCameraRelativeRotationは、呼び出し元が必要とする対象または計算結果を返します。
    FRotator GetCameraRelativeRotation() const;

    //AR装備中のメッシュが、肩越しカメラの画面中央側を向いているか確認します。
    UFUNCTION(BlueprintPure, Category = "Animation|Rifle")
    bool IsRifleFacingScreenCenter(float _toleranceDegrees = 2.0f) const;

    //AnimBP Speed 変数に接続する移動速度
    UFUNCTION(BlueprintCallable, Category = "Animation")
    float GetMoveSpeed() const { return GetVelocity().Size2D(); }

    //待機状態を返します。
    UFUNCTION(BlueprintCallable, Category = "Animation")
    EIdleState GetIdleState() const { return m_idleState; }

    //ストレーフ用: キャラクター前方への速度成分
    UFUNCTION(BlueprintCallable, Category = "Animation")
    float GetVelocityForward() const;

    //ストレーフ用: キャラクター右方向への速度成分
    UFUNCTION(BlueprintCallable, Category = "Animation")
    float GetVelocityRight() const;

    //体力を返します。
    UFUNCTION(BlueprintCallable, Category = "HP")
    float GetHP() { return m_hp; }

    UFUNCTION(BlueprintCallable, Category = "Weapon",
              meta = (DeprecatedFunction, DeprecationMessage = "Use the built-in Pistol/AR/Knife slot system instead."))
    void AttachWeapon(TSubclassOf<AActor> _weaponClass);

  public:
    //m_pEquippedWeaponをゲーム処理から参照できるように管理します。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    //m_pEquippedWeaponをゲーム処理から参照できるように管理します。
    TObjectPtr<AActor> m_pEquippedWeapon;

    //userWidgetNormalをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "UI")
    //userWidgetNormalをゲーム処理から参照できるように管理します。
    TSubclassOf<UUserWidget> m_userWidgetNormal;

    //userWidgetAimをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "UI")
    //userWidgetAimをゲーム処理から参照できるように管理します。
    TSubclassOf<UUserWidget> m_userWidgetAim;

    //layerHPクラスの操作に使用する参照です。
    UPROPERTY(EditAnywhere, Category = "UI")
    //layerHPクラスの操作に使用する参照です。
    TSubclassOf<UUserWidget> m_playerHPClass;

    //旧リロードWidgetとの互換性を保つために残しています。
    //現在の弾数UIは UAmmoHUDWidget がC++側で生成します。
    UPROPERTY(EditAnywhere, Category = "UI", meta = (DeprecatedProperty, DeprecationMessage = "UAmmoHUDWidgetへ移行済みです。"))
    //onReloadクラスをゲーム処理から参照できるように管理します。
    TSubclassOf<UUserWidget> m_onReloadClass;

    //weaponCarouselクラスをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "UI|Weapon Carousel")
    //weaponCarouselクラスをゲーム処理から参照できるように管理します。
    TSubclassOf<UWeaponCarouselWidget> m_weaponCarouselClass;

    //meleeWeaponクラスをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Weapon|Classes")
    //meleeWeaponクラスをゲーム処理から参照できるように管理します。
    TSubclassOf<AMeleeWeapon> m_meleeWeaponClass;

    //arWeaponクラスをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Weapon|Classes")
    //arWeaponクラスをゲーム処理から参照できるように管理します。
    TSubclassOf<AARWeapon> m_arWeaponClass;

    //死亡モンタージュを保持します。
    UPROPERTY(EditAnywhere, Category = "Animation")
    UAnimMontage* m_pDeathMontage;

    //エイムモンタージュを保持します。
    UPROPERTY(EditAnywhere, Category = "Animation")
    UAnimMontage* m_pAimMontage;

    //リロードモンタージュを保持します。
    UPROPERTY(EditAnywhere, Category = "Animation")
    UAnimMontage* m_pReloadMontage;

    //PistolReloadアニメーションの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Animation|Reload")
    //PistolReloadアニメーションの操作に使用する参照です。
    TObjectPtr<UAnimSequenceBase> m_pPistolReloadAnimation;

    //ライフルリロードアニメーションを保持します。
    UPROPERTY(EditDefaultsOnly, Category = "Animation|Reload")
    TObjectPtr<UAnimSequenceBase> m_pRifleReloadAnimation;

    //LandingMontageの操作に使用する参照です。
    UPROPERTY(EditAnywhere, Category = "Animation")
    //LandingMontageの操作に使用する参照です。
    UAnimMontage* m_pLandingMontage;

    //HealMontageの操作に使用する参照です。
    UPROPERTY(EditAnywhere, Category = "Animation")
    //HealMontageの操作に使用する参照です。
    UAnimMontage* m_pHealMontage;

    //ナイフ攻撃用Montageです。
    //設計上、攻撃アニメーションは武器ではなくプレイヤー側で管理します。
    UPROPERTY(EditAnywhere, Category = "Animation|Melee")
    TArray<TObjectPtr<UAnimMontage>> m_knifeAttackMontages;

    //SwitchToPistolMontageの操作に使用する参照です。
    UPROPERTY(EditAnywhere, Category = "Animation")
    //SwitchToPistolMontageの操作に使用する参照です。
    UAnimMontage* m_pSwitchToPistolMontage;

    //SwitchToARMontageの操作に使用する参照です。
    UPROPERTY(EditAnywhere, Category = "Animation")
    //SwitchToARMontageの操作に使用する参照です。
    UAnimMontage* m_pSwitchToARMontage;

    //SwitchToKnifeMontageの操作に使用する参照です。
    UPROPERTY(EditAnywhere, Category = "Animation")
    //SwitchToKnifeMontageの操作に使用する参照です。
    UAnimMontage* m_pSwitchToKnifeMontage;

    //gameOverLevelNameをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Death")
    FName m_gameOverLevelName = TEXT("GameOver");

    //death画面Delayを秒単位で指定します。
    UPROPERTY(EditAnywhere, Category = "Death", meta = (ClampMin = "0.0"))
    //death画面Delayを秒単位で指定します。
    float m_deathScreenDelay = 0.05f;

    //idleTimeoutSecondsを秒単位で指定します。
    UPROPERTY(EditAnywhere, Category = "Animation|Idle")
    //idleTimeoutSecondsを秒単位で指定します。
    float m_idleTimeoutSeconds = 8.f;


    //reload速度Scaleの調整値です。
    UPROPERTY(EditAnywhere, Category = "Move")
    //reload速度Scaleの調整値です。
    float m_reloadSpeedScale = 0.4f;

    //switchWeapon速度Scaleの調整値です。
    UPROPERTY(EditAnywhere, Category = "Move")
    //switchWeapon速度Scaleの調整値です。
    float m_switchWeaponSpeedScale = 0.3f;

    //jumpStartMoveLockTimeを秒単位で指定します。
    UPROPERTY(EditAnywhere, Category = "Move|Animation Lock", meta = (ClampMin = "0.0"))
    //jumpStartMoveLockTimeを秒単位で指定します。
    float m_jumpStartMoveLockTime = 0.12f;

    //landingFallbackUnlockTimeを秒単位で指定します。
    UPROPERTY(EditAnywhere, Category = "Move|Animation Lock", meta = (ClampMin = "0.0"))
    //landingFallbackUnlockTimeを秒単位で指定します。
    float m_landingFallbackUnlockTime = 0.45f;

    //実時間で感じるヒットストップの長さです。60fpsでは約4フレームになります。
    UPROPERTY(EditAnywhere, Category = "Combat|Feedback", meta = (ClampMin = "0.01", ClampMax = "0.15"))
    //killHitStop時間を秒単位で指定します。
    float m_killHitStopDuration = 0.065f;

    //ヒットストップ中のゲーム速度です。0にはせず、エフェクトと入力更新を残します。
    UPROPERTY(EditAnywhere, Category = "Combat|Feedback", meta = (ClampMin = "0.01", ClampMax = "0.5"))
    //killHitStopTimeDilationを秒単位で指定します。
    float m_killHitStopTimeDilation = 0.08f;

  private:
    //カメラを更新します。
    void UpdateCamera(float _deltaTime);
    //UpdateMoveは、引数の内容をゲーム中の状態または表示へ反映します。
    void UpdateMove(float _deltaTime);
    void AlignRifleVisualFacing();
    //ジャンプを更新します。
    void UpdateJump(float _deltaTime);
    //クロスヘアを更新します。
    void UpdateCrosshair(float _deltaTime);
    //待機状態を更新します。
    void UpdateIdleState(float _deltaTime);
    //ジャンプを完了します。
    void EndJump();
    void SwitchToPistol();
    void SwitchToAR();
    void SwitchToKnife();
    bool SwitchWeaponToSlot(EWeaponSlot _slot);
    void CycleNextWeapon();
    void CyclePreviousWeapon();
    //OnWeaponWheelは、名前が示すイベント通知を受けて関連するゲーム状態を更新します。
    void OnWeaponWheel(float _wheelValue);
    void RebuildWeaponDisplayOrder();
    void MoveWeaponSlotToBottom(EWeaponSlot _slot);
    void RefreshWeaponCarousel();
    bool IsWeaponAvailable(EWeaponSlot _slot) const;
    //GetWeaponForSlotは、呼び出し元が必要とする対象または計算結果を返します。
    AWeaponBase* GetWeaponForSlot(EWeaponSlot _slot) const;
    //切り替え武器完了状態を処理します。
    void OnSwitchWeaponFinished();

    //リロード武器を処理します。
    void ReloadWeapon();
    //ReloadMontageEndedの通知を受けてゲーム状態へ反映します。
    UFUNCTION()
    void HandleReloadMontageEnded(UAnimMontage* _montage, bool _bInterrupted);
    //アイテムを取得し、効果を反映します。
    void PickupItem(EItemType _type, float _value);
    //待機タイマーを解除します。
    void ResetIdleTimer();
    bool CanAcceptMoveInput() const;
    //BeginDeathSequenceは、名前が示す動作を開始するための初期状態を整えます。
    void BeginDeathSequence();
    void FreezeDeathPose();
    void RestoreKillHitStop();
    void Cam_RotatePitch(float _value);
    void Cam_RotateYaw(float _value);
    void Chara_MoveForward(float _value);
    void Chara_MoveRight(float _value);
    void JumpStart();
    //エイムを処理します。
    void StartAim();
    //StopAimは、名前が示す動作を終了し、継続中の状態を解除します。
    void StopAim();
    //StopAttackは、名前が示す動作を終了し、継続中の状態を解除します。
    void StopAttack();

    //Rifle Idleから照準姿勢への遷移を開始します。
    void BeginRifleAimTransition();

    //Aiming Idle開始後に保留中の射撃を処理します。
    void HandleRifleAimReady();

    //Aiming Idleの実再生を確認してから一発発射します。
    void FirePendingRifleShot();

    //トリガー保持中のAR連射を継続します。
    void ContinueRifleAutomaticFire();

    //AR射撃を終了してRifle Idleへ戻します。
    void FinishRifleAttack();

    //構えMontageが中断された場合を含め、照準姿勢を保証します。
    void EnsurePersistentRifleAim();

    //Rifle Aiming Idleが実際に再生中か確認します。
    bool IsRifleReadyToFire() const;
    bool ShouldUsePersistentRifleAim() const;
    void RefreshRifleCombatAim();
    //射撃終了後にライフルの構えを解除します。
    void EndRifleCombatAim();
    //StopRifleAimPoseは、名前が示す動作を終了し、継続中の状態を解除します。
    void StopRifleAimPose(float _blendOutTime = 0.12f);
    void SwapCrosshairWidget();
    void ControlHUDVisiblity();
    //UseHealItemは、名前が示す装備または機能を使用する処理を開始します。
    void UseHealItem();

  private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    //SpringArmの操作に使用する参照です。
    USpringArmComponent* m_pSpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    //カメラの操作に使用する参照です。
    UCameraComponent* m_pCamera;

    //暗い場所でプレイヤーの輪郭だけを補う、BPから調整可能な追従ライトです。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lighting", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UPointLightComponent> m_pPlayerFillLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    //音声Componentの操作に使用する参照です。
    TObjectPtr<UPlayerAudioComponent> m_pAudioComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    //RifleアニメーションComponentの操作に使用する参照です。
    TObjectPtr<UPlayerRifleAnimationComponent> m_pRifleAnimationComponent;

    //cameraPitchLimitをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Camera")
    //cameraPitchLimitをゲーム処理から参照できるように管理します。
    FVector2D m_cameraPitchLimit;


    //move速度の調整値です。
    UPROPERTY(EditAnywhere, Category = "Move")
    //move速度の調整値です。
    float m_moveSpeed;

    //gravityをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Jump")
    //gravityをゲーム処理から参照できるように管理します。
    float m_gravity;

    //jumpPowerをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Jump")
    //jumpPowerをゲーム処理から参照できるように管理します。
    float m_jumpPower;

    //m_weaponSocketNameをゲーム処理から参照できるように管理します。
    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    //m_weaponSocketNameをゲーム処理から参照できるように管理します。
    FName m_weaponSocketName;

    //defaultFOVをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Camera|Aim")
    //defaultFOVをゲーム処理から参照できるように管理します。
    float m_defaultFOV;

    //aimFOVをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Camera|Aim")
    //aimFOVをゲーム処理から参照できるように管理します。
    float m_aimFOV;

    //aimInterp速度の調整値です。
    UPROPERTY(EditAnywhere, Category = "Camera|Aim")
    //aimInterp速度の調整値です。
    float m_aimInterpSpeed;

    //defaultカメラSocketOffsetをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Camera|Aim")
    //defaultカメラSocketOffsetをゲーム処理から参照できるように管理します。
    FVector m_defaultCameraSocketOffset;

    //rifleAimカメラSocketOffsetをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Camera|Aim")
    //rifleAimカメラSocketOffsetをゲーム処理から参照できるように管理します。
    FVector m_rifleAimCameraSocketOffset;

    //defaultカメラArmLengthをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Camera|Aim")
    //defaultカメラArmLengthをゲーム処理から参照できるように管理します。
    float m_defaultCameraArmLength;

    //rifleAimカメラArmLengthをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Camera|Aim")
    //rifleAimカメラArmLengthをゲーム処理から参照できるように管理します。
    float m_rifleAimCameraArmLength;

    //aimカメラPositionInterp速度の調整値です。
    UPROPERTY(EditAnywhere, Category = "Camera|Aim")
    //aimカメラPositionInterp速度の調整値です。
    float m_aimCameraPositionInterpSpeed;

    //肩越しカメラ使用時に、ARを画面中央へ向けるための水平方向補正です。
    UPROPERTY(EditAnywhere, Category = "Camera|Aim", meta = (ClampMin = "-30.0", ClampMax = "30.0"))
    float m_rifleAimInwardYawOffset;

    //第三者視点で上半身が画面中央へ向いて見える、AR専用の視覚収束距離です。
    UPROPERTY(EditAnywhere, Category = "Camera|Aim", meta = (ClampMin = "200.0", ClampMax = "2000.0"))
    //rifleVisualConvergence距離の調整値です。
    float m_rifleVisualConvergenceDistance;

    UPROPERTY() UCombatCrosshairWidget* m_pUserWidget;
    UPROPERTY() UUserWidget* m_pPlayerHp;
    UPROPERTY() UAmmoHUDWidget* m_pReloadUI;
    UPROPERTY() UWeaponCarouselWidget* m_pWeaponCarousel;
    UPROPERTY() UEnemyLocatorWidget* m_pEnemyLocatorWidget;
    UPROPERTY() AWeaponBase* m_pPistolWeapon;
    UPROPERTY() AMeleeWeapon* m_pKnifeWeapon;
    UPROPERTY() AARWeapon* m_pARWeapon;

  private:
    //現在のスロットを保持します。
    EWeaponSlot m_currentSlot;
    //待機状態を保持します。
    EIdleState m_idleState;

    //切り替え武器タイマーを保持します。
    FTimerHandle m_switchWeaponTimer;
    //死亡タイマーを保持します。
    FTimerHandle m_deathTimer;
    //deathPoseFreezeTimerを秒単位で指定します。
    FTimerHandle m_deathPoseFreezeTimer;
    //healFallbackTimerを秒単位で指定します。
    FTimerHandle m_healFallbackTimer;
    //movementLockTimerを秒単位で指定します。
    FTimerHandle m_movementLockTimer;
    //rifleCombatAimTimerを秒単位で指定します。
    FTimerHandle m_rifleCombatAimTimer;
    //rifleAutomaticFireTimerを秒単位で指定します。
    FTimerHandle m_rifleAutomaticFireTimer;
    //killHitStopTimerを秒単位で指定します。
    FTimerHandle m_killHitStopTimer;

    //healItem数を管理します。
    int32 m_healItemCount;
    //crosshairMaxDistをゲーム処理から参照できるように管理します。
    float m_crosshairMaxDist;
    //キャラクター移動を保持します。
    FVector2D m_charaMovement;
    //カメラ回転を保持します。
    FVector2D m_cameraRotation;
    //defaultMeshRelative回転をゲーム処理から参照できるように管理します。
    FRotator m_defaultMeshRelativeRotation;
    //idleTimerAccumを秒単位で指定します。
    float m_idleTimerAccum = 0.f;
    //weaponDisplayOrderをゲーム処理から参照できるように管理します。
    TArray<EWeaponSlot> m_weaponDisplayOrder;

    //ジャンプ中を保持します。
    bool m_bJumping;
    //CanControlかを示します。
    bool m_bCanControl;
    //IsAimingかを示します。
    bool m_bIsAiming;
    //IsFiringRifleかを示します。
    bool m_bIsFiringRifle;
    //AR戦闘時の照準姿勢かを示します。
    bool m_bRifleCombatAim;
    //PendingRifleShotかを示します。
    bool m_bPendingRifleShot;
    //RifleTriggerHeldかを示します。
    bool m_bRifleTriggerHeld;
    //IsHealingかを示します。
    bool m_bIsHealing;
    //HasARかを示します。
    bool m_bHasAR;
    //IsReloadingAnimかを示します。
    bool m_bIsReloadingAnim;
    //IsSwitchingWeaponかを示します。
    bool m_bIsSwitchingWeapon;
    //移動LockedByアニメーションかを示します。
    bool m_bMovementLockedByAnimation;
    //クロスヘア非表示状態を保持します。
    bool m_bCrosshairSuppressed;
    //IsDeadかを示します。
    bool m_bIsDead;
    //GameOverRequestedかを示します。
    bool m_bGameOverRequested = false;
};
