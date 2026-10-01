#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerAudioComponent.generated.h"

//AGunWeaponは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class AGunWeapon;
//UAudioComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UAudioComponent;
//USkeletalMeshComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class USkeletalMeshComponent;
//USoundBaseは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class USoundBase;

//プレイヤー固有の音声再生とアニメーション同期を管理するコンポーネント。
//足音はAnimNotifyから呼び、リロード音はMontageの残り時間に合わせて
//Pitchを調整することで、アニメーションと同じフレームで終了させます。
UCLASS(ClassGroup = (Player), meta = (BlueprintSpawnableComponent))
class ZOMBIEATTACK_API UPlayerAudioComponent : public UActorComponent
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //プレイヤー音声コンポーネントを処理します。
    UPlayerAudioComponent();

    //PlayFootstepは、名前が示す動作を開始するための初期状態を整えます。
    void PlayFootstep();
    //PlayLandingは、名前が示す動作を開始するための初期状態を整えます。
    void PlayLanding();
    //PlayReloadSoundは、名前が示す動作を開始するための初期状態を整えます。
    void PlayReloadSound(AGunWeapon* _gunWeapon, float _targetDuration);
    //StopReloadSoundは、名前が示す動作を終了し、継続中の状態を解除します。
    void StopReloadSound(float _fadeOutDuration = 0.03f);

  private:
    bool CanPlayFootstep() const;

  private:
    //足音サウンドの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Player Audio|Movement")
    //足音サウンドの操作に使用する参照です。
    TObjectPtr<USoundBase> m_pFootstepSound;

    //Landingサウンドの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Player Audio|Movement")
    //Landingサウンドの操作に使用する参照です。
    TObjectPtr<USoundBase> m_pLandingSound;

    //minimum足音速度の調整値です。
    UPROPERTY(EditDefaultsOnly, Category = "Player Audio|Movement", meta = (ClampMin = "0.0"))
    //minimum足音速度の調整値です。
    float m_minimumFootstepSpeed;

    //minimum足音間隔を秒単位で指定します。
    UPROPERTY(EditDefaultsOnly, Category = "Player Audio|Movement", meta = (ClampMin = "0.01"))
    //minimum足音間隔を秒単位で指定します。
    float m_minimumFootstepInterval;

    //リロード音声コンポーネントを保持します。
    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> m_pReloadAudioComponent;

    //last足音Timeを秒単位で指定します。
    double m_lastFootstepTime;
};
