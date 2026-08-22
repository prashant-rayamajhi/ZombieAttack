#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Bullet.generated.h"

//UStaticMeshComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UStaticMeshComponent;
//UProjectileMovementComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UProjectileMovementComponent;
//UNiagaraSystemは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UNiagaraSystem;
//UNiagaraComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UNiagaraComponent;

//Bulletの動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API ABullet : public AActor
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    ABullet();

  public:
    //毎フレームの更新を行います。
    virtual void Tick(float _deltaTime) override;

    //銃側から渡された方向へ弾を飛ばす関数
    void SetMoveDirection(const FVector& _moveDirection);

    //プレイヤーや武器へ弾が当たらないようにする関数
    void IgnoreActor(AActor* _actor);

    //銃側から命中エフェクトを受け取る関数
    void SetImpactEffect(UNiagaraSystem* _impactEffect, float _impactEffectLifetime);

    //カメラレイで命中を確定させる場合、弾Actor側のダメージをOFFにする関数
    void SetDamageEnabled(bool _bEnabled);

  public:
    //弾の移動コンポーネント
    UPROPERTY(VisibleAnywhere, Category = "Movement")
    TObjectPtr<UProjectileMovementComponent> m_pProjectileComp;

    //弾の見た目と当たり判定
    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UStaticMeshComponent> m_pBulletMesh;

  protected:
    //弾が何かに当たった時に呼ばれる関数
    UFUNCTION()
    void OnHit(UPrimitiveComponent* _hitComp, AActor* _otherActor, UPrimitiveComponent* _otherComp, FVector _normalImpulse, const FHitResult& _hit);

  private:
    //命中エフェクトを一度だけ出す関数
    void SpawnImpactEffectOnce(const FHitResult& _hit);

    //Niagaraを安全に停止して破棄する関数
    void DestroyNiagaraAfterDelay(UNiagaraComponent* _component, float _delaySeconds) const;

  private:
    //弾のダメージ量
    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    float m_damage;

    //カメラレイ命中方式では、弾Actorの二重ダメージを防ぐためfalseにする
    UPROPERTY(VisibleAnywhere, Category = "Damage")
    bool m_bDamageEnabled;

    //弾の移動方向
    UPROPERTY(VisibleAnywhere, Category = "Movement")
    FVector m_moveDirection;

    //1発の弾で命中処理を複数回実行しないためのフラグ
    UPROPERTY(VisibleAnywhere, Category = "Hit")
    bool m_bHasHit;

    //命中時に再生するNiagara
    UPROPERTY()
    TObjectPtr<UNiagaraSystem> m_impactEffect;

    //命中エフェクトを消すまでの時間
    UPROPERTY()
    float m_impactEffectLifetime;
};
