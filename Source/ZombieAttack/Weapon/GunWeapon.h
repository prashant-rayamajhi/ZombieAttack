#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "GunWeapon.generated.h"

//ABulletは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class ABullet;
//UNiagaraSystemは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UNiagaraSystem;
//UNiagaraComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UNiagaraComponent;
//USoundBaseは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class USoundBase;

//Onリロード完了状態を通知するデリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadFinished);

//銃武器の動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API AGunWeapon : public AWeaponBase
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //銃武器を処理します。
    AGunWeapon();

  public:
    //UseWeaponは、名前が示す装備または機能を使用する処理を開始します。
    virtual void UseWeapon() override;

    //リロードを処理します。
    UFUNCTION(BlueprintCallable, Category = "Reload")
    void Reload();

    //リロードを完了します。
    void FinishReload();
    void CancelReload() { m_bIsReloading = false; }

    //現在の弾薬を返します。
    int32 GetCurrentAmmo() const { return m_currentClipAmmo; }
    int32 GetTotalAmmo() const { return m_currentTotalAmmo; }
    int32 GetMaxClipAmmo() const { return m_maxClipAmmo; }
    int32 GetTotalMaxAmmo() const { return m_totalMaxAmmo; }
    float GetFireRate() const { return FMath::Max(0.02f, m_fireRate); }
    //リロード効果音を返します。
    USoundBase* GetReloadSound() const { return m_pReloadSound; }

    //AddAmmoは、名前が示す対象を既存の状態または一覧へ追加します。
    virtual void AddAmmo(float _amount) override;

    //リロード可能かどうかを返します。
    bool CanReload() const;

  public:
    //OnReloadFinishedをゲーム処理から参照できるように管理します。
    UPROPERTY(BlueprintAssignable, Category = "Weapon|Ammo")
    //OnReloadFinishedをゲーム処理から参照できるように管理します。
    FOnReloadFinished OnReloadFinished;

  protected:
    //射程距離です。
    UPROPERTY(EditAnywhere, Category = "Weapon|Aim")
    float m_range;

    //武器ごとの自動射撃間隔です。
    UPROPERTY(EditAnywhere, Category = "Weapon")
    float m_fireRate;

    //壁や地面などを調べるTrace半径です。0ならLineTraceです。
    UPROPERTY(EditAnywhere, Category = "Weapon|Aim", meta = (ClampMin = "0.0"))
    float m_aimTraceSphereRadius;

    //近距離の敵を拾いやすくするPawn専用Trace半径です。
    UPROPERTY(EditAnywhere, Category = "Weapon|Aim", meta = (ClampMin = "0.0"))
    float m_pawnAimTraceSphereRadius;

    //カメラレイで当たったActorに即時ダメージを入れるかどうかです。
    UPROPERTY(EditAnywhere, Category = "Weapon|Aim")
    bool m_bUseCameraRayDamage;


    //muzzleFlashをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Effect")
    //muzzleFlashをゲーム処理から参照できるように管理します。
    TObjectPtr<UNiagaraSystem> m_muzzleFlash;

    //impactEffectをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Effect")
    //impactEffectをゲーム処理から参照できるように管理します。
    TObjectPtr<UNiagaraSystem> m_impactEffect;

    //muzzleFlashLifetimeを秒単位で指定します。
    UPROPERTY(EditAnywhere, Category = "Effect", meta = (ClampMin = "0.01"))
    //muzzleFlashLifetimeを秒単位で指定します。
    float m_muzzleFlashLifetime;

    //muzzleFlashScaleをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Effect", meta = (ClampMin = "0.01", ClampMax = "2.0"))
    //muzzleFlashScaleをゲーム処理から参照できるように管理します。
    float m_muzzleFlashScale;

    //impactEffectLifetimeを秒単位で指定します。
    UPROPERTY(EditAnywhere, Category = "Effect", meta = (ClampMin = "0.01"))
    //impactEffectLifetimeを秒単位で指定します。
    float m_impactEffectLifetime;

    //生成する弾クラスです。BP_GunWeaponでBP_Bulletを設定してください。
    UPROPERTY(EditAnywhere, Category = "Weapon", meta = (DisplayName = "Bullet Class"))
    TSubclassOf<ABullet> m_bulletClass;

    //maxClip弾薬をゲーム処理から参照できるように管理します。
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo")
    //maxClip弾薬をゲーム処理から参照できるように管理します。
    int32 m_maxClipAmmo;

    //totalMax弾薬をゲーム処理から参照できるように管理します。
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
    //totalMax弾薬をゲーム処理から参照できるように管理します。
    int32 m_totalMaxAmmo;

    //currentClip弾薬をゲーム処理から参照できるように管理します。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Ammo")
    //currentClip弾薬をゲーム処理から参照できるように管理します。
    int32 m_currentClipAmmo;

    //current合計弾薬をゲーム処理から参照できるように管理します。
    UPROPERTY(VisibleAnywhere, Category = "Weapon|Ammo")
    //current合計弾薬をゲーム処理から参照できるように管理します。
    int32 m_currentTotalAmmo;

    //IsReloadingかを示します。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reload")
    //IsReloadingかを示します。
    bool m_bIsReloading;

    //Fireサウンドの操作に使用する参照です。
    UPROPERTY(EditAnywhere, Category = "Sound")
    //Fireサウンドの操作に使用する参照です。
    TObjectPtr<USoundBase> m_pFireSound;

    //Emptyサウンドの操作に使用する参照です。
    UPROPERTY(EditAnywhere, Category = "Sound")
    //Emptyサウンドの操作に使用する参照です。
    TObjectPtr<USoundBase> m_pEmptySound;

    //リロード効果音を保持します。
    UPROPERTY(EditAnywhere, Category = "Sound")
    TObjectPtr<USoundBase> m_pReloadSound;

  private:
    //カメラエイム対象を返します。
    bool GetCameraAimTarget(FVector& _outCameraStart, FVector& _outCameraEnd, FVector& _outTargetPoint,
                            //constをゲーム処理から参照できるように管理します。
                            FHitResult& _outHitResult) const;

    //ApplyCameraRayDamageは、引数の内容をゲーム中の状態または表示へ反映します。
    void ApplyCameraRayDamage(const FHitResult& _hitResult, const FVector& _damageDirection, AController* _ownerController);

    //カメラRayImpactエフェクトを作成します。
    void SpawnCameraRayImpactEffect(const FHitResult& _hitResult);


    //MuzzleFlashを作成します。
    void SpawnMuzzleFlash(const FVector& _location, const FRotator& _rotation);
    //NiagaraAfterDelayを解除します。
    void DestroyNiagaraAfterDelay(UNiagaraComponent* _component, float _delaySeconds) const;
};
