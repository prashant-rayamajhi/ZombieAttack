#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "MeleeWeapon.generated.h"

//USoundBaseは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class USoundBase;
//UAnimMontageは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAnimMontage;
//UNiagaraSystemは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UNiagaraSystem;

//Melee武器の動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API AMeleeWeapon : public AWeaponBase
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    AMeleeWeapon();

    //UseWeaponは、名前が示す装備または機能を使用する処理を開始します。
    virtual void UseWeapon() override;

    //AnimNotify_MeleeHitから呼ばれます。
    void ExecuteHit();

    //AnimNotify_MeleeComboBranchから呼ばれます。
    void TryContinueComboFromNotify();

    //コンボを強制終了します。
    void ResetCombo();

  protected:
    //攻撃範囲を保持します。
    UPROPERTY(EditAnywhere, Category = "Combat")
    float m_attackRange;

    //攻撃半径を保持します。
    UPROPERTY(EditAnywhere, Category = "Combat")
    float m_attackRadius;

    //attackCoolDownをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Combat")
    //attackCoolDownをゲーム処理から参照できるように管理します。
    float m_attackCoolDown;

    //comboWindow時間を秒単位で指定します。
    UPROPERTY(EditAnywhere, Category = "Combat")
    //comboWindow時間を秒単位で指定します。
    float m_comboWindowDuration;

    //maxCombo数を管理します。
    UPROPERTY(EditAnywhere, Category = "Combat")
    //maxCombo数を管理します。
    int32 m_maxComboCount;

    //互換用の予備設定です。基本はPlayerChara側の m_knifeAttackMontages を使います。
    UPROPERTY(EditAnywhere, Category = "Animation|Fallback")
    TArray<TObjectPtr<UAnimMontage>> m_attackMontages;

    //Swingサウンドの操作に使用する参照です。
    UPROPERTY(EditAnywhere, Category = "Sound")
    //Swingサウンドの操作に使用する参照です。
    TObjectPtr<USoundBase> m_pSwingSound;

    //Hitサウンドの操作に使用する参照です。
    UPROPERTY(EditAnywhere, Category = "Sound")
    //Hitサウンドの操作に使用する参照です。
    TObjectPtr<USoundBase> m_pHitSound;

    //視覚フィードバックをデータ設定に分離し、Blueprintから既定素材を差し替えられるようにします。
    UPROPERTY(EditDefaultsOnly, Category = "VFX")
    TObjectPtr<UNiagaraSystem> m_pSwingVFX;

    //HitVFXの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "VFX")
    //HitVFXの操作に使用する参照です。
    TObjectPtr<UNiagaraSystem> m_pHitVFX;

  private:
    //PlayCurrentComboMontageは、名前が示す動作を開始するための初期状態を整えます。
    void PlayCurrentComboMontage();
    //PerformSweepは、名前が示す判定または動作を実行します。
    void PerformSweep();

  private:
    //currentComboIndexを管理します。
    int32 m_currentComboIndex;
    //Is攻撃ingかを示します。
    bool m_bIsAttacking;
    //ComboQueuedかを示します。
    bool m_bComboQueued;
    //HitExecutedThisSwingかを示します。
    bool m_bHitExecutedThisSwing;

    //comboResetTimerを秒単位で指定します。
    FTimerHandle m_comboResetTimer;
    //cooldownTimerを秒単位で指定します。
    FTimerHandle m_cooldownTimer;
};
