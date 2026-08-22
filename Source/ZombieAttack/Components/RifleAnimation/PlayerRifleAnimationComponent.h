#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RifleAnimationTypes.h"
#include "PlayerRifleAnimationComponent.generated.h"

//UAnimMontageは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAnimMontage;
//UAnimSequenceBaseは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAnimSequenceBase;
//USkeletalMeshComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class USkeletalMeshComponent;

//OnライフルエイムReadyを通知するデリゲート
DECLARE_MULTICAST_DELEGATE(FOnRifleAimReady);

//ARの照準姿勢と、発砲可能になるまでの状態を管理します。
//AnimBP側の遷移だけに依存せず、AR装備中はRifle Aiming Idleを基準姿勢として
//使用します。これにより、見た目がRifle Idleのまま発砲する状態を防ぎます。
UCLASS(ClassGroup = (Player), meta = (BlueprintSpawnableComponent))
class ZOMBIEATTACK_API UPlayerRifleAnimationComponent : public UActorComponent
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //プレイヤーライフルアニメーションコンポーネントを処理します。
    UPlayerRifleAnimationComponent();

    //ARの照準待機姿勢を開始します。
    void BeginAimSequence(USkeletalMeshComponent* _mesh);

    //AnimNotifyから照準完了を通知します。
    void CompleteAimFromNotify();

    //照準姿勢を停止して通常状態へ戻します。
    void StopAimSequence(float _blendOutTime = 0.12f);

    //リロード中も上半身レイヤーを有効に保ちます。
    void SetReloading(USkeletalMeshComponent* _mesh);

    //エイム状態を返します。
    ERifleAimState GetAimState() const { return m_aimState; }
    //IsAimReadyPoseActiveは、名前が示す条件の成立可否を呼び出し元へ返します。
    bool IsAimReadyPoseActive() const;
    FOnRifleAimReady& OnAimReady() { return m_onAimReady; }

  protected:
    virtual void TickComponent(float _deltaTime, ELevelTick _tickType,
                               //overrideの操作に使用する参照です。
                               FActorComponentTickFunction* _tickFunction) override;

    //ゲーム終了時に登録済みの通知とタイマーを解除します。
    virtual void EndPlay(const EEndPlayReason::Type _endPlayReason) override;

  private:
    //統一したRifle Aiming Idle姿勢を開始します。
    void PlayRaiseTransition();

    //Aiming Idleの再生確認後にのみReadyへ遷移します。
    void EnterAimReady();

    //指定したアニメーションをUpperBodyスロットで再生します。
    UAnimMontage* PlaySequence(UAnimSequenceBase* _animation, float _blendInTime, float _blendOutTime, float _playRate,
                               //loop数を管理します。
                               int32 _loopCount = 1);

    //AnimBPの上半身合成ウェイトを更新します。
    void SetUpperBodyBlendWeight(float _blendWeight) const;

  private:
    //ライフル待機アニメーションを保持します。
    UPROPERTY(EditDefaultsOnly, Category = "Animation|Rifle")
    //従来のAR待機姿勢
    TObjectPtr<UAnimSequenceBase> m_pRifleIdleAnimation;

    //RifleDownToAimアニメーションの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Animation|Rifle")
    //従来の照準遷移
    TObjectPtr<UAnimSequenceBase> m_pRifleDownToAimAnimation;

    //ARの照準ingIdleアニメーションの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Animation|Rifle")
    //ARの統一照準姿勢
    TObjectPtr<UAnimSequenceBase> m_pRifleAimingIdleAnimation;

    //UseUnifiedAimingPoseかを示します。
    UPROPERTY(EditDefaultsOnly, Category = "Animation|Rifle")
    //AR装備中をAiming Idleへ統一するか
    bool b_mUseUnifiedAimingPose;

    //raisePlayRateをゲーム処理から参照できるように管理します。
    UPROPERTY(EditDefaultsOnly, Category = "Animation|Rifle", meta = (ClampMin = "0.1"))
    //互換用の照準遷移再生速度
    float m_raisePlayRate;

    //実行状態モンタージュを保持します。
    UPROPERTY(Transient)
    //現在の動的Montage
    TObjectPtr<UAnimMontage> m_pActiveMontage;

    //対象メッシュを保持します。
    TWeakObjectPtr<USkeletalMeshComponent> m_pTargetMesh;
    //エイム状態を保持します。
    ERifleAimState m_aimState;
    //sequenceTimerを秒単位で指定します。
    FTimerHandle m_sequenceTimer;
    //onAim準備完了をゲーム処理から参照できるように管理します。
    FOnRifleAimReady m_onAimReady;
};
