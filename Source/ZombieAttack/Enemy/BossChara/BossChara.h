#pragma once

#include "CoreMinimal.h"
#include "../EnemyChara.h"
#include "ZombieAttack/AIController/BossAITypes.h"
#include "BossChara.generated.h"

//前方宣言
class USoundBase;
//UAnimMontageは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAnimMontage;
//APickUpBaseは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class APickUpBase;
//UNiagaraSystemは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UNiagaraSystem;

//ボスの攻撃パターンの列挙型
UENUM(BlueprintType)
enum class EBossAttackPattern : uint8
{
    //隙の少ない連続攻撃
    LightCombo UMETA(DisplayName = "Light Combo"),
    //地面を叩く範囲攻撃
    PowerSlam UMETA(DisplayName = "Power Slam"),
    //前方へ走り込む突進攻撃
    ChargeRush UMETA(DisplayName = "Charge Rush"),
    //間合いを作る後方回避
    BackStep UMETA(DisplayName = "Back Step")
};

//ボスキャラクターの動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API ABossChara : public AEnemyChara
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //コンストラクタ
    ABossChara();

    //毎フレーム呼ばれる関数
    virtual void Tick(float _deltaTime) override;

    //ダメージを受けたときに呼ばれる関数
    virtual float TakeDamage(float _damageAmount, FDamageEvent const& _damageEvent, AController* _eventInstigator,
                             //overrideの操作に使用する参照です。
                             AActor* _damageCauser) override;

    //既存の手動攻撃開始用です。現在の状況から攻撃パターンを選んで開始
    void RequestAttack();

    //中ボス/ラスボスでAIプリセットを切り替える
    virtual EBossAIArchetype GetBossAIArchetype() const;

    //BossAIControllerが次の行動を選んでよいか確認するための関数
    virtual bool CanPerformTacticalAction() const;

    //BossAIControllerから指定された攻撃パターンを開始する関数
    virtual bool RequestSpecificAttack(EBossAttackPattern _pattern, const FVector& _targetLocation);

    //モンタージュが設定されている攻撃だけをAIの候補にします。
    bool SupportsAttackPattern(EBossAttackPattern _pattern) const;

    //Utility AI側から各行動を選択可能か確認します。
    bool SupportsTacticalAction(EBossTacticalAction _action) const;

    //BossUtilityAIのスコアをボス種別ごとに微調整する関数
    virtual float ModifyUtilityScore(EBossTacticalAction _action, const FBossDecisionContext& _context,
                                     //constをゲーム処理から参照できるように管理します。
                                     float _baseScore) const;

    //UtilityAIが距離評価に使う近接攻撃距離
    virtual float GetBossMeleeRange() const;

    //足元中心の衝撃波が届く間合いを、AIが使うカプセル間の距離へ換算する。
    float GetSlamStartRange() const;

    //UtilityAIが攻撃を要求し始める距離関数
    virtual float GetBossAttackRequestRange() const;

    //0.0 - 1.0 のHP割合
    virtual float GetHealthRatio() const;

    //AnimNotify_BossAttackHitから呼ばれる攻撃判定
    virtual void PerformBossAttackHit();
    //通常敵用の命中通知でも、ボス固有の攻撃判定へ渡す
    virtual void PerformAttackHit(float _damageMultiplier = 1.f) override { PerformBossAttackHit(); }
    //通常連撃は通知区間の手足だけに当たり判定を付け、衝撃波と分ける。
    virtual void SetAttackCollisionEnabled(bool _bEnabled) override;

    //BeginAttackVFXWindowは、名前が示す動作を開始するための初期状態を整えます。
    virtual void BeginAttackVFXWindow() override;

    //敵の攻撃がヒットしたときに呼ばれる関数
    virtual void OnEnemyAttackHit(AActor* _hitActor) override;

    //コンボ攻撃の分岐をAnimNotifyから要求する関数
    virtual void RequestComboBranchFromNotify() override;

    //ボスの攻撃パターンを取得する関数
    UFUNCTION(BlueprintCallable, Category = "Boss")
    EBossAttackPattern GetCurrentPattern() const { return m_currentPattern; }

    //ボスが第2フェーズに入っているかどうかを取得する関数
    UFUNCTION(BlueprintCallable, Category = "Boss")
    bool IsPhaseTwo() const { return m_bPhaseTwo; }

    //ボスが攻撃中かどうかを取得する関数
    UFUNCTION(BlueprintCallable, Category = "Boss")
    bool IsTransitioning() const { return m_bIsTransitioning; }

  protected:
    //死亡後にフェーズ移行や攻撃タイマーが再発火しないよう破棄する
    virtual void PlayDeathAnimationAndDie() override;
    //ゲーム開始時に呼ばれる関数
    virtual void BeginPlay() override;

    //ボスの攻撃ロジックを使用するかどうかを取得する関数
    virtual bool ShouldUseHandleAttackLogic() const override { return false; }

    //フェーズ２に移行するHP割合の閾値
    UPROPERTY(EditAnywhere, Category = "Boss|Phase")
    float m_phaseTwoThreshold;

    //ボスの攻撃パターンを選択する関数
    UPROPERTY(EditAnywhere, Category = "Boss|Animation")
    TObjectPtr<UAnimMontage> m_pLightComboMontage;

    //ボスの攻撃パターンを選択する関数
    UPROPERTY(EditAnywhere, Category = "Boss|Animation")
    TObjectPtr<UAnimMontage> m_pPowerSlamMontage;

    //ボスの攻撃パターンを選択する関数
    UPROPERTY(EditAnywhere, Category = "Boss|Animation")
    TObjectPtr<UAnimMontage> m_pChargeRushMontage;

    //ボスの攻撃パターンを選択する関数
    UPROPERTY(EditAnywhere, Category = "Boss|Animation")
    TObjectPtr<UAnimMontage> m_pBackStepMontage;

    //ボスの攻撃パターンを選択する関数
    UPROPERTY(EditAnywhere, Category = "Boss|Animation", meta = (DisplayName = "Combo Attack Montages"))
    //combo攻撃Montagesをゲーム処理から参照できるように管理します。
    TArray<TObjectPtr<UAnimMontage>> m_comboAttackMontages;

    //ボスのフェーズ移行時のアニメーションモンタージュ
    UPROPERTY(EditAnywhere, Category = "Boss|Animation", meta = (DisplayName = "Phase Transition Montage"))
    //PhaseTransitionMontageの操作に使用する参照です。
    TObjectPtr<UAnimMontage> m_pPhaseTransitionMontage;

    //ボスの咆哮時のサウンド
    UPROPERTY(EditAnywhere, Category = "Boss|Sound")
    TObjectPtr<USoundBase> m_pRoarSound;

    //ボスの攻撃ヒット時のサウンド
    UPROPERTY(EditAnywhere, Category = "Boss|Sound")
    TObjectPtr<USoundBase> m_pImpactSound;

    //武器のドロップクラス
    UPROPERTY(EditAnywhere, Category = "Boss|Drop")
    TSubclassOf<APickUpBase> m_pWeaponDropClass;

    //武器のドロップ値
    UPROPERTY(EditAnywhere, Category = "Boss|Drop")
    float m_weaponDropValue;

    //ボスの攻撃クールダウン時間
    UPROPERTY(EditAnywhere, Category = "Boss|Combat")
    float m_attackCooldown;

    //ボスの第2フェーズの攻撃クールダウン時間
    UPROPERTY(EditAnywhere, Category = "Boss|Combat")
    float m_phaseTwoAttackCooldown;

    //ボスの近接攻撃距離
    UPROPERTY(EditAnywhere, Category = "Boss|Combat")
    float m_meleeRange;

    //ボスが攻撃を要求し始める距離
    UPROPERTY(EditAnywhere, Category = "Boss|Combat")
    float m_chargeDistance;

    //Slamエフェクトを生成するボス前方の距離です。
    UPROPERTY(EditDefaultsOnly, Category = "Boss|VFX", meta = (ClampMin = "0.0"))
    float m_powerSlamForwardOffset;

    //Slamモンタージュ終了の何秒前にエフェクトを生成するかを指定します。
    UPROPERTY(EditDefaultsOnly, Category = "Boss|VFX", meta = (ClampMin = "0.0"))
    float m_powerSlamEffectLeadTime;

    //攻撃パターンごとに識別しやすいエフェクトを割り当て、Blueprintから既定値を差し替えられるようにします。
    UPROPERTY(EditDefaultsOnly, Category = "Boss|VFX")
    TObjectPtr<UNiagaraSystem> m_pComboImpactVFX;

    //PowerSlamVFXの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Boss|VFX")
    //PowerSlamVFXの操作に使用する参照です。
    TObjectPtr<UNiagaraSystem> m_pPowerSlamVFX;

    //ChargeVFXの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Boss|VFX")
    //ChargeVFXの操作に使用する参照です。
    TObjectPtr<UNiagaraSystem> m_pChargeVFX;

    //BackStepVFXの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Boss|VFX")
    //BackStepVFXの操作に使用する参照です。
    TObjectPtr<UNiagaraSystem> m_pBackStepVFX;

    //PhaseTransitionVFXの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Boss|VFX")
    //PhaseTransitionVFXの操作に使用する参照です。
    TObjectPtr<UNiagaraSystem> m_pPhaseTransitionVFX;

  private:
    //攻撃、コンボ、フェーズ移行に関係する予約をまとめて取り消す
    void ClearCombatTimers();
    //終了通知の重複で硬直時間が延びるのを防ぐ
    bool m_bRecovering = false;
    //ボスの攻撃パターンを選択する関数
    EBossAttackPattern ChoosePattern() const;

    //ボスの攻撃パターンを実行する関数
    void ExecutePattern(EBossAttackPattern _pattern);

    //ボスの攻撃パターンを実行する関数
    void DoLightCombo();
    //DoPowerSlamは、名前が示す攻撃または移動を実行します。
    void DoPowerSlam();
    //DoChargeRushは、名前が示す攻撃または移動を実行します。
    void DoChargeRush();
    //突進方向を開始時に固定し、衝突を保った歩行移動で走り込む。
    void UpdateChargeRush(float _deltaTime);
    //DoBackStepは、名前が示す攻撃または移動を実行します。
    void DoBackStep();
    //PowerSlamエフェクトを作成します。
    void SpawnPowerSlamEffect();
    //ExecuteLightComboAreaAttackは、名前が示す攻撃または動作を実行します。
    void ExecuteLightComboAreaAttack();
    bool ResolveGroundEffectTransform(float _forwardOffset, FVector& _outLocation,
                                      //constをゲーム処理から参照できるように管理します。
                                      FRotator& _outRotation) const;
    bool ResolveGroundEffectTransformAt(const FVector& _traceCenter, FVector& _outLocation,
                                        //constをゲーム処理から参照できるように管理します。
                                        FRotator& _outRotation) const;
    //StartComboAttackは、名前が示す動作を開始するための初期状態を整えます。
    void StartComboAttack();
    //PlayComboStepは、名前が示す動作を開始するための初期状態を整えます。
    void PlayComboStep();
    //ボス攻撃を完了します。
    void FinishBossAttack();

    //ボスのフェーズ移行処理を開始する関数
    virtual void DropBossLoot();

    //ボスの死亡処理を行う関数
    virtual void Die() override;

  private:
    //ボスの現在の攻撃パターン
    EBossAttackPattern m_currentPattern;

    //ボスが第2フェーズに入っているかどうか
    bool m_bPhaseTwo;

    //ボスが攻撃中かどうか
    bool m_bAttacking;

    //ボスがフェーズ移行中かどうか
    bool m_bIsTransitioning;

    //ボスのフェーズ移行がトリガーされたかどうか
    bool m_bPhaseTransitionTriggered;

    //ボスのコンボ攻撃がヒットしたかどうか
    bool m_bComboHitConfirmed;
    //LightComboAreaResolvedThisStepかを示します。
    bool m_bLightComboAreaResolvedThisStep;
    //PowerSlamEffectSpawnedかを示します。
    bool m_bPowerSlamEffectSpawned;

    //ボスのコンボ攻撃の現在のステップインデックス
    int32 m_currentComboIndex;

    //Utility AIが算出した予測迎撃地点を保持し、突進先のずれを防ぎます。
    //突進攻撃がプレイヤーの更新前または現在位置へ不用意に戻ることを防ぎます。
    FVector m_requestedTargetLocation;
    //突進開始時の方向を保ち、走行中にプレイヤーを追尾して曲がらないようにする。
    FVector m_rushDirection = FVector::ZeroVector;
    //走行開始からの時間で、予備動作と接触判定の区間を分ける。
    float m_rushElapsed = 0.0f;
    //走行クリップ二周期の長さに突進の移動時間を合わせる。
    float m_rushDuration = 0.0f;
    //同じ突進で接触判定を繰り返し初期化しないための開始記録。
    bool m_bRushContactStarted = false;
    //スラムの接地通知が重なっても、一回の攻撃で二重にダメージを与えない。
    bool m_bSlamHitResolved = false;

    //ボスの攻撃クールダウンタイマー
    FTimerHandle m_attackCooldownTimer;
    //attackアニメーションSafetyTimerを秒単位で指定します。
    FTimerHandle m_attackAnimationSafetyTimer;
    //owerSlamEffectTimerの操作に使用する参照です。
    FTimerHandle m_powerSlamEffectTimer;
    //lightComboEffectTimerを秒単位で指定します。
    FTimerHandle m_lightComboEffectTimer;
    //chargeTimerを秒単位で指定します。
    FTimerHandle m_chargeTimer;
    //haseTransitionTimerの操作に使用する参照です。
    FTimerHandle m_phaseTransitionTimer;
};
