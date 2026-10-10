#pragma once

#include "CoreMinimal.h"
#include "../Character/BaseCharacter.h"
#include "EnemyChara.generated.h"

//前方宣言
class APlayerChara;
//UWidgetComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UWidgetComponent;
//UAnimMontageは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAnimMontage;
//APickUpBaseは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class APickUpBase;
//USoundBaseは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class USoundBase;
//USoundAttenuationは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class USoundAttenuation;
//USoundConcurrencyは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class USoundConcurrency;
//UBoxComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UBoxComponent;
//UNiagaraSystemは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UNiagaraSystem;
//UNiagaraComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UNiagaraComponent;
//UEnemyHitFeedbackComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UEnemyHitFeedbackComponent;
class UAnimSequence;

//デリゲート宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthChanged);
//Defeated敵を通知するデリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDefeated, AEnemyChara*, DefeatedEnemy);

//敵の移動状態の列挙型
UENUM(BlueprintType)
enum class EEnemyMoveState : uint8
{
    //周囲を警戒しながら巡回する状態
    Patrol UMETA(DisplayName = "Patrol"),
    //プレイヤーを見つけて戦う状態
    Combat UMETA(DisplayName = "Combat")
};

//通常敵の移動、攻撃、被ダメージ、死亡処理を管理する基底クラスです。
//AIは行動選択を担当し、このクラスはアニメーションとゲーム上の当たり判定を
//同じタイミングで実行する責務を持ちます。
UCLASS()
class ZOMBIEATTACK_API AEnemyChara : public ABaseCharacter
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //コンストラクタ
    AEnemyChara();

    //移動評価が使う敵種別の姿勢を返す
    UAnimSequence* GetIdleAnimation() const { return m_idleAnimation; }
    UAnimSequence* GetWalkAnimation() const { return m_walkAnimation; }
    UAnimSequence* GetRunAnimation() const { return m_runAnimation; }
    //左右の回り込みで、前進走行を横滑りさせず専用の足運びを返す。
    UAnimSequence* GetStrafeLeftAnimation() const { return m_strafeLeft; }
    UAnimSequence* GetStrafeRightAnimation() const { return m_strafeRight; }
    //初回発見時の咆哮を再生し、移動を止める秒数を返す
    float PlayAlertAnimation();
    //壁、高低差、背後への空振りを除外して近接攻撃の届く範囲を判定する
    bool CanHitPlayer(float _range, float _minFacing = 0.15f) const;
    //通知に指定した手足へ判定を付け、存在しない骨なら判定を無効にする。
    bool PrepareAttackContact(FName _bone);
    //今回の攻撃で使う手足の位置をエフェクトと命中判定で共有する。
    FVector GetAttackContactLocation() const;
    //有効な攻撃区間で手足が通過した範囲を調べ、一振りにつき一度だけ命中を通知する。
    void TraceAttackContact();
    //生成順に依存せず、コントローラーが発見したプレイヤーを攻撃対象へ渡す。
    void SetCombatTarget(APlayerChara* _player) { m_pPlayerChara = _player; }
    //視覚で追跡している相手を攻撃とエフェクトの命中判定でも使う。
    APlayerChara* GetCombatTarget() const { return m_pPlayerChara; }

    //毎フレーム呼ばれる関数
    virtual void Tick(float _deltaTime) override;

    //ダメージを受けたときに呼ばれる関数
    virtual float TakeDamage(float _damageAmount, const FDamageEvent& _damageEvent, AController* _eventInstigator,
                             //overrideの操作に使用する参照です。
                             AActor* _damageCauser) override;

    //敵のHPが変化したときに呼ばれるデリゲート
    UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnHealthChanged m_onHealthChanged;

    //敵が倒されたときに呼ばれるデリゲート
    UPROPERTY(BlueprintAssignable, Category = "Enemy")
    FOnEnemyDefeated m_onEnemyDefeated;

    //HPを取得する関数
    UFUNCTION(BlueprintPure, Category = "Health")
    float GetHP() const { return m_hp; }

    //最大HPを取得する関数
    UFUNCTION(BlueprintPure, Category = "Health")
    float GetMaxHP() const { return m_maxHp; }

    //攻撃中かどうかを取得する関数
    UFUNCTION(BlueprintPure, Category = "Animation")
    bool IsAttacking() const { return m_bIsAttacking; }

    //死亡しているかどうかを取得する関数
    UFUNCTION(BlueprintPure, Category = "Animation")
    bool IsDead() const { return m_bIsDead; }

    //敵の移動状態を取得する関数
    UFUNCTION(BlueprintPure, Category = "Animation")
    EEnemyMoveState GetMoveState() const { return m_moveState; }

    //敵の速度を取得する関数
    UFUNCTION(BlueprintPure, Category = "Animation")
    float GetVelocitySize() const { return GetVelocity().Size2D(); }

    //攻撃範囲を取得する関数
    UFUNCTION(BlueprintPure, Category = "AI|Combat")
    float GetDesiredCombatRange() const { return FMath::Max(140.f, m_attackRange + 80.f); }
    //手足の接触攻撃を始めるカプセル間の距離を、行動選択と共通にする。
    float GetContactAttackRange() const { return m_attackRange; }

    //現在の攻撃判定を一度だけ適用します。
    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void PerformAttackHit(float _damageMultiplier = 1.f);

    //攻撃クールダウンを解除し、次の攻撃要求を許可します。
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void TriggerAttack();

    //攻撃Collision有効をゲーム内の対象へ反映します。
    UFUNCTION(BlueprintCallable, Category = "Attack")
    virtual void SetAttackCollisionEnabled(bool _bEnabled);

    //攻撃HitForNewSwingを解除します。
    UFUNCTION(BlueprintCallable, Category = "Attack")
    void ResetAttackHitForNewSwing();

    //AnimNotifyStateの有効区間だけ、手元の攻撃エフェクトを表示します。
    UFUNCTION(BlueprintCallable, Category = "Attack|VFX")
    virtual void BeginAttackVFXWindow();

    //攻撃判定が終了したら、手元の攻撃エフェクトも確実に停止します。
    UFUNCTION(BlueprintCallable, Category = "Attack|VFX")
    virtual void EndAttackVFXWindow();

    //攻撃MontageのNotifyを受け、敵の連続攻撃を次段へ進めます。
    UFUNCTION(BlueprintCallable, Category = "Attack")
    virtual void RequestComboBranchFromNotify();

    //OnEnemyAttackHitは、名前が示すイベント通知を受けて関連するゲーム状態を更新します。
    virtual void OnEnemyAttackHit(AActor* _hitActor);

    //赤いアウトラインを有効化する関数
    void EnableRedOutline(bool _bEnable);

    //赤いアウトラインの色を設定する関数
    void SetRedOutlineColor(bool _bEnable);

    //プレイヤーに見られていない時間をリセットする関数
    void ResetNotSeenTimer();

    //銃弾の命中位置と方向を、敵共通の命中演出へ渡します。
    void PlayBulletImpactFeedback(const FHitResult& _hitResult, const FVector& _shotDirection);

    //検索支援のブースト状態を設定する関数
    UFUNCTION(BlueprintCallable, Category = "Visibility Assist")
    void SetSearchAssistBoosted(bool _bBoosted);

    //赤いアウトラインが有効かどうかを取得する関数
    UFUNCTION(BlueprintPure, Category = "Visibility Assist")
    bool IsRedOutlineEnabled() const { return m_bRedOutlineEnabled; }

  protected:
    //通常敵とボスが同じ攻撃中フラグを使い、移動と通知の受付条件を揃える
    void SetCombatActionActive(bool _bActive) { m_bIsAttacking = _bActive; }
    //派生ボスのタイマーを停止してから共通の死亡姿勢へ切り替える
    virtual void PlayDeathAnimationAndDie();
    //派生クラスが同じSkeletonのIdle、Walk、Run、Alertをまとめて設定する
    void SetLocomotionAssets(const TCHAR* _folder, const TCHAR* _idle, const TCHAR* _walk, const TCHAR* _run, const TCHAR* _alert);
    //旧AnimBPの状態遷移に依存しない共通アニメーションを開始する
    void InitializeEnemyAnimation();
    //各敵の停止姿勢
    UPROPERTY(EditDefaultsOnly, Category = "Animation|Locomotion")
    TObjectPtr<UAnimSequence> m_idleAnimation;
    //毎秒125cmを基準とする前進姿勢
    UPROPERTY(EditDefaultsOnly, Category = "Animation|Locomotion")
    TObjectPtr<UAnimSequence> m_walkAnimation;
    //毎秒500cmを基準とする走行姿勢
    UPROPERTY(EditDefaultsOnly, Category = "Animation|Locomotion")
    TObjectPtr<UAnimSequence> m_runAnimation;
    //この敵の骨格に合わせた左移動クリップ。
    UPROPERTY(EditDefaultsOnly, Category = "Animation|Locomotion")
    TObjectPtr<UAnimSequence> m_strafeLeft;
    //この敵の骨格に合わせた右移動クリップ。
    UPROPERTY(EditDefaultsOnly, Category = "Animation|Locomotion")
    TObjectPtr<UAnimSequence> m_strafeRight;
    //プレイヤーを初めて見つけた時に一度だけ再生する姿勢
    UPROPERTY(EditDefaultsOnly, Category = "Animation|Locomotion")
    TObjectPtr<UAnimSequence> m_alertAnimation;
    //現在の敵Skeletonで安全に再生できるMontageか確認します。
    bool IsMontageCompatible(const UAnimMontage* _montage) const;

    //ゲーム開始時に呼ばれる関数
    virtual void BeginPlay() override;

    //攻撃ロジックを使用するかどうかを取得する関数
    virtual bool ShouldUseHandleAttackLogic() const { return true; }

    //死亡処理を行う関数
    virtual void Die() override;

    //アイテムをドロップする関数
    virtual void DropItem();

    //敵のサウンドを再生する関数
    void PlayEnemySound(USoundBase* _pSound, float _volumeMultiplier = 1.f) const;
    //プレイヤーキャラクターを返します。
    APlayerChara* GetPlayerCharacter() const;

    //敵のHPを表示するウィジェットコンポーネント
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UWidgetComponent> m_pHealthComp;

    //死亡時のアニメーションモンタージュ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
    TObjectPtr<UAnimMontage> m_pDeathMontage;

    //攻撃時のアニメーションモンタージュ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
    TObjectPtr<UAnimMontage> m_pAttackMontage;
    //通常敵の攻撃候補。同じSkeletonと接触通知を持つモンタージュだけを選ぶ。
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TArray<TObjectPtr<UAnimMontage>> m_attackChoices;
    //終了通知が別のモンタージュと混ざらないよう、今回再生した攻撃を保持する。
    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> m_activeAttack;

    //ドロップするアイテムのクラス
    UPROPERTY(EditAnywhere, Category = "Drop")
    TSubclassOf<APickUpBase> m_pPickUpClass;

    //ドロップするアイテムの確率
    UPROPERTY(EditAnywhere, Category = "Drop", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float m_regularDropProbability;

    //ドロップするアイテムの価値
    UPROPERTY(EditAnywhere, Category = "Sound")
    TObjectPtr<USoundBase> m_pPatrolSound;

    //ドロップするアイテムの価値
    UPROPERTY(EditAnywhere, Category = "Sound")
    TObjectPtr<USoundBase> m_pAttackSound;

    //ドロップするアイテムの価値
    UPROPERTY(EditAnywhere, Category = "Sound")
    TObjectPtr<USoundBase> m_pAlertSound;

    //ドロップするアイテムの価値
    UPROPERTY(EditAnywhere, Category = "Sound")
    TObjectPtr<USoundBase> m_pDeathSound;

    //敵のサウンドの減衰設定
    UPROPERTY(EditAnywhere, Category = "Sound")
    TObjectPtr<USoundAttenuation> m_pSoundAttenuation;

    //敵のサウンドの同時再生制御設定
    UPROPERTY(EditAnywhere, Category = "Sound")
    TObjectPtr<USoundConcurrency> m_pSoundConcurrency;

    //パトロール時のサウンド再生間隔の最小値
    UPROPERTY(EditAnywhere, Category = "Sound", meta = (ClampMin = "0.1"))
    float m_patrolSoundIntervalMin;

    //パトロール時のサウンド再生間隔の最大値
    UPROPERTY(EditAnywhere, Category = "Sound", meta = (ClampMin = "0.1"))
    float m_patrolSoundIntervalMax;

    //パトロール時のサウンドの聞こえる範囲
    UPROPERTY(EditAnywhere, Category = "Sound", meta = (ClampMin = "0.0"))
    float m_patrolSoundHearingRange;

    //パトロール時の歩行速度
    UPROPERTY(EditAnywhere, Category = "Move", meta = (ClampMin = "0.0"))
    float m_patrolWalkSpeed;

    //追跡時の走行速度
    UPROPERTY(EditAnywhere, Category = "Move", meta = (ClampMin = "0.0"))
    float m_chaseRunSpeed;

    //検索支援機能を有効化するかどうか
    UPROPERTY(EditAnywhere, Category = "Visibility Assist")
    bool m_bEnableVisibilityAssist;

    //検索支援機能が有効な場合の、プレイヤーに見られていない時間のしきい値
    UPROPERTY(EditAnywhere, Category = "Visibility Assist", meta = (ClampMin = "0.0"))
    float m_notSeenAlertTime;

    //検索支援機能が有効な場合の、ブースト状態でのプレイヤーに見られていない時間のしきい値
    UPROPERTY(EditAnywhere, Category = "Visibility Assist", meta = (ClampMin = "0.0"))
    float m_boostedNotSeenAlertTime;

    //検索支援機能が有効な場合の、プレイヤーとの距離のしきい値
    UPROPERTY(EditAnywhere, Category = "Visibility Assist", meta = (ClampMin = "100.0"))
    float m_visibilityAssistMaxDistance;

    //検索支援機能が有効な場合の、プレイヤーの視線方向とのドット積のしきい値
    UPROPERTY(EditAnywhere, Category = "Visibility Assist", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    //visibilityAssistFocusDotをゲーム処理から参照できるように管理します。
    float m_visibilityAssistFocusDot;

    //検索支援機能が有効な場合の、プレイヤーに見られていない時間のチェック間隔
    UPROPERTY(EditAnywhere, Category = "Visibility Assist", meta = (ClampMin = "0.05"))
    float m_visibilityAssistCheckInterval;

    //検索支援機能が有効な場合の、赤いアウトラインを解除するまでの遅延時間
    UPROPERTY(EditAnywhere, Category = "Visibility Assist", meta = (ClampMin = "0.0"))
    float m_outlineReleaseDelay;

    //攻撃範囲
    UPROPERTY(EditAnywhere, Category = "Combat", meta = (ClampMin = "0.0"))
    float m_attackRange;

    //攻撃間隔
    UPROPERTY(EditAnywhere, Category = "Combat", meta = (ClampMin = "0.0"))
    float m_attackInterval;

    //攻撃のヒットダメージ
    UPROPERTY(EditAnywhere, Category = "Combat", meta = (ClampMin = "0"))
    int32 m_hitDamage;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack", meta = (AllowPrivateAccess = "true"))
    //攻撃Collisionの操作に使用する参照です。
    TObjectPtr<UBoxComponent> m_pAttackCollision;

    //攻撃WindupVFXの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Attack|VFX")
    //攻撃WindupVFXの操作に使用する参照です。
    TObjectPtr<UNiagaraSystem> m_pAttackWindupVFX;

    //攻撃ImpactVFXの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Attack|VFX")
    //攻撃ImpactVFXの操作に使用する参照です。
    TObjectPtr<UNiagaraSystem> m_pAttackImpactVFX;

    //Active攻撃VFXの操作に使用する参照です。
    UPROPERTY(Transient)
    //Active攻撃VFXの操作に使用する参照です。
    TObjectPtr<UNiagaraComponent> m_pActiveAttackVFX;

  private:
    //直前の手足の位置から線分を作り、フレーム間の接触も判定する。
    FVector m_previousContact = FVector::ZeroVector;
    //判定開始直後は骨の付け替えを攻撃軌跡に含めない。
    bool m_contactReady = false;
    //攻撃CollisionOverlapが発生したときの処理を行います。
    UFUNCTION()
    void OnAttackCollisionOverlap(UPrimitiveComponent* _overlappedComp, AActor* _otherActor, UPrimitiveComponent* _otherComp, int32 _otherBodyIndex,
                                  bool _bFromSweep, const FHitResult& _sweepResult);

    //プレイヤーとの距離と攻撃間隔から、通常攻撃を開始できるか判定します。
    void HandleAttackLogic(float _deltaTime);

    //移動を停止して互換性確認済みの攻撃Montageを開始します。
    void BeginAttack();

    //攻撃状態を終了し、AIの追跡処理へ戻します。
    void EndAttack();

    //敵種に対応した死亡Sequenceを直接再生します。
    //中断を含むモンタージュ終了時に、移動停止と攻撃判定を解除する
    void HandleAttackMontageEnded(UAnimMontage* _montage, bool _bInterrupted);

    //死亡アニメーションの最終姿勢を保持します。
    void FreezeDeathPose();

    //敵のHPバーを一時的に表示する関数
    void ShowHealthBarTemporarily(float _displaySeconds = 3.f);

    //パトロールサウンドを再生して次の再生をスケジュールする関数
    void PlayPatrolSoundAndReschedule();

    //プレイヤーまでの有効距離を取得する関数
    float GetEffectiveDistanceToPlayer() const;

    //プレイヤーに見られていない時間を更新する関数
    void UpdateAlertOutline(float _deltaTime);

    //プレイヤーに見られているかどうかを判定する関数
    bool IsClearlyVisibleToPlayer() const;

    //実際の移動速度とアニメーションBPの速度を同期する
    void SynchronizeLocomotionAnimation();

  private:
    //HitFeedbackComponentの操作に使用する参照です。
    UPROPERTY(VisibleAnywhere, Category = "Hit Feedback")
    //銃弾命中演出
    TObjectPtr<UEnemyHitFeedbackComponent> m_pHitFeedbackComponent;

    //敵の現在のHP
    UPROPERTY(Transient)
    TObjectPtr<APlayerChara> m_pPlayerChara;

    //現在の移動状態
    EEnemyMoveState m_moveState;

    //攻撃中かどうかのフラグ
    bool m_bIsDead;

    //攻撃中かどうかのフラグ
    bool m_bIsAttacking;

    //攻撃がヒットしたかどうかのフラグ
    bool m_bHitAppliedThisAttack;

    //死亡処理が完了したかどうかのフラグ
    bool m_bRedOutlineEnabled;

    //死亡処理が完了したかどうかのフラグ
    bool m_bDefeatBroadcast;

    //死亡処理が完了したかどうかのフラグ
    bool m_bSearchAssistBoosted;

    //攻撃のクールダウン時間の残り
    float m_attackCooldownRemaining;

    //プレイヤーに見られていない時間のカウンター
    float m_notSeenTimer;

    //プレイヤーに見られていない時間のチェック間隔のカウンター
    float m_visibilityAssistCheckAccumulator;

    //赤いアウトラインを解除するまでの遅延時間のカウンター
    float m_outlineReleaseAccumulator;

    //攻撃HitThisSwingかを示します。
    bool m_bAttackHitThisSwing;

    //attackEndTimerを秒単位で指定します。
    FTimerHandle m_attackEndTimer;
    //deathTimerHandleを秒単位で指定します。
    FTimerHandle m_deathTimerHandle;
    //deathPoseFreezeTimerを秒単位で指定します。
    FTimerHandle m_deathPoseFreezeTimer;
    //atrolサウンドTimerの操作に使用する参照です。
    FTimerHandle m_patrolSoundTimer;
    //healthBarHideTimerを秒単位で指定します。
    FTimerHandle m_healthBarHideTimer;
};
