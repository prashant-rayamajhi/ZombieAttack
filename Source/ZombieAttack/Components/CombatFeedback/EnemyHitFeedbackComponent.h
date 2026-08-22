#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyHitFeedbackComponent.generated.h"

//UMaterialInterfaceは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UMaterialInterface;
//UStaticMeshは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UStaticMesh;
//UStaticMeshComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UStaticMeshComponent;

//敵が銃弾を受けた際の血痕と飛沫を一か所で管理するコンポーネントです。
//武器ごとに同じ演出処理を持たせず、敵の種類が増えた場合もこのコンポーネントを
//再利用できるようにします。
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class ZOMBIEATTACK_API UEnemyHitFeedbackComponent : public UActorComponent
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    UEnemyHitFeedbackComponent();

    virtual void TickComponent(float _deltaTime, ELevelTick _tickType,
                               //overrideの操作に使用する参照です。
                               FActorComponentTickFunction* _thisTickFunction) override;

    //命中面へ血痕を残し、弾の進行方向に沿った血液飛沫を生成します。
    void PlayBulletImpact(const FHitResult& _hitResult, const FVector& _shotDirection);

    //自動検証で再生中の飛沫数を確認します。
    int32 GetActiveBloodDropletCount() const { return m_activeBloodDroplets.Num(); }

  private:
    //実行状態BloodDropletに必要な値をまとめた構造体
    struct FActiveBloodDroplet
    {
        //コンポーネントを保持します。
        TWeakObjectPtr<UStaticMeshComponent> m_pComponent;
        //速度を保持します。
        FVector m_velocity = FVector::ZeroVector;
        //initialScaleをゲーム処理から参照できるように管理します。
        FVector m_initialScale = FVector::OneVector;
        //remainingLifeをゲーム処理から参照できるように管理します。
        float m_remainingLife = 0.0f;
        //totalLifeをゲーム処理から参照できるように管理します。
        float m_totalLife = 0.0f;
    };

    //実行状態BloodSpriteに必要な値をまとめた構造体
    struct FActiveBloodSprite
    {
        //コンポーネントを保持します。
        TWeakObjectPtr<UStaticMeshComponent> m_pComponent;
        //initialScaleをゲーム処理から参照できるように管理します。
        FVector m_initialScale = FVector::OneVector;
        //remainingLifeをゲーム処理から参照できるように管理します。
        float m_remainingLife = 0.0f;
        //totalLifeをゲーム処理から参照できるように管理します。
        float m_totalLife = 0.0f;
    };

    //命中面へ血痕デカールを生成します。
    void SpawnBloodDecal(const FHitResult& _hitResult) const;

    //命中点から短時間だけ飛ぶ血液の粒を生成します。
    void SpawnBloodDroplets(const FHitResult& _hitResult, const FVector& _shotDirection);

    //命中した瞬間に読み取りやすい血液スプライトを生成します。
    void SpawnBloodImpactSprite(const FHitResult& _hitResult, const FVector& _shotDirection);

    //BloodDecalMaterialの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback|Blood")
    //血痕デカール用Material
    TObjectPtr<UMaterialInterface> m_pBloodDecalMaterial;

    //BloodDropletMaterialの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback|Blood")
    //血液飛沫用Material
    TObjectPtr<UMaterialInterface> m_pBloodDropletMaterial;

    //BloodImpactSpriteMaterialの操作に使用する参照です。
    UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback|Blood")
    //瞬間表示用Material
    TObjectPtr<UMaterialInterface> m_pBloodImpactSpriteMaterial;

    //DropletMeshの操作に使用する参照です。
    UPROPERTY()
    //血液飛沫に使用する軽量Mesh
    TObjectPtr<UStaticMesh> m_pDropletMesh;

    //ImpactPlaneMeshの操作に使用する参照です。
    UPROPERTY()
    //血液スプライト用Plane
    TObjectPtr<UStaticMesh> m_pImpactPlaneMesh;

    //droplet数を管理します。
    UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback|Blood", meta = (ClampMin = "1"))
    //一回の命中で生成する飛沫数
    int32 m_dropletCount;

    //decalSizeをゲーム処理から参照できるように管理します。
    UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback|Blood", meta = (ClampMin = "1.0"))
    //血痕の表示サイズ
    float m_decalSize;

    //decalLifeSecondsをゲーム処理から参照できるように管理します。
    UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback|Blood", meta = (ClampMin = "0.1"))
    //血痕を残す時間
    float m_decalLifeSeconds;

    //dropletLifeSecondsをゲーム処理から参照できるように管理します。
    UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback|Blood", meta = (ClampMin = "0.05"))
    //飛沫を表示する時間
    float m_dropletLifeSeconds;

    //droplet速度Minの調整値です。
    UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback|Blood", meta = (ClampMin = "0.0"))
    //飛沫の最低速度
    float m_dropletSpeedMin;

    //droplet速度Maxの調整値です。
    UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback|Blood", meta = (ClampMin = "0.0"))
    //飛沫の最高速度
    float m_dropletSpeedMax;

    //dropletGravityをゲーム処理から参照できるように管理します。
    UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback|Blood", meta = (ClampMin = "0.0"))
    //飛沫へ加える重力
    float m_dropletGravity;

    //impactSpriteLifeSecondsをゲーム処理から参照できるように管理します。
    UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback|Blood", meta = (ClampMin = "0.05"))
    //血液スプライトを表示する時間
    float m_impactSpriteLifeSeconds;

    //impactSpriteScaleをゲーム処理から参照できるように管理します。
    UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback|Blood", meta = (ClampMin = "0.05"))
    //血液スプライトの基本サイズ
    float m_impactSpriteScale;

    //再生中の血液飛沫
    TArray<FActiveBloodDroplet> m_activeBloodDroplets;
    //再生中の血液スプライト
    TArray<FActiveBloodSprite> m_activeBloodSprites;
};
