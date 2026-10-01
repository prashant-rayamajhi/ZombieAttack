
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GoalActor.generated.h"

//前方宣言
class USphereComponent;
//UStaticMeshComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UStaticMeshComponent;
//UUserWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UUserWidget;
//APlayerCharaは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class APlayerChara;
//UNiagaraComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UNiagaraComponent;
//UNiagaraSystemは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UNiagaraSystem;
//UPointLightComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UPointLightComponent;
//UMaterialInstanceDynamicは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UMaterialInstanceDynamic;
//UTextRenderComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UTextRenderComponent;

//GoalActorの動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API AGoalActor : public AActor
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //コンストラクタ
    AGoalActor();

    //コンストラクタ
    virtual void OnConstruction(const FTransform& _transform) override;
    //毎フレームの更新を行います。
    virtual void Tick(float _deltaTime) override;

    //ゴールを有効化する
    UFUNCTION(BlueprintCallable, Category = "Goal")
    void ActivateGoal();

    //ゴールが有効化されているかどうかを取得する
    UFUNCTION(BlueprintPure, Category = "Goal")
    bool IsActivated() const { return m_bActivated; }

    //ゴールにプレイヤーが到達した時に呼ばれる関数
    UFUNCTION(BlueprintImplementableEvent, Category = "Goal|Presentation")
    void BP_OnGoalActivated();

    //ゴールにプレイヤーが到達した時に呼ばれる関数
    UFUNCTION(BlueprintImplementableEvent, Category = "Goal|Presentation")
    void BP_OnGoalReached(APlayerChara* _player);

  protected:
    //初期化処理
    virtual void PostInitializeComponents() override;
    //ゲーム開始時の初期設定を行います。
    virtual void BeginPlay() override;

    //ゴールにプレイヤーが到達した時に呼ばれる関数
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* _overlappedComponent, AActor* _otherActor, UPrimitiveComponent* _otherComponent, int32 _otherBodyIndex,
                        bool _bFromSweep, const FHitResult& _sweepResult);

    //SphereComponentの参照
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    //SphereCompの操作に使用する参照です。
    TObjectPtr<USphereComponent> m_pSphereComp;

    //StaticMeshComponentの参照
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    //MeshCompの操作に使用する参照です。
    TObjectPtr<UStaticMeshComponent> m_pMeshComp;

    //出口の台座・支柱・矢印で共有する、金属テクスチャ付きの素材。
    UPROPERTY(EditDefaultsOnly, Category = "Goal|Presentation")
    TObjectPtr<UMaterialInterface> m_beaconMaterial;

    //NiagaraComponentの参照
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    //MagicCircleCompの操作に使用する参照です。
    TObjectPtr<UNiagaraComponent> m_pMagicCircleComp;

    //PointLightComponentの参照
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    //ゴールLightCompの操作に使用する参照です。
    TObjectPtr<UPointLightComponent> m_pGoalLightComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    //BeaconBaseCompの操作に使用する参照です。
    TObjectPtr<UStaticMeshComponent> m_pBeaconBaseComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    //BeaconColumnCompの操作に使用する参照です。
    TObjectPtr<UStaticMeshComponent> m_pBeaconColumnComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    //BeaconArrowCompの操作に使用する参照です。
    TObjectPtr<UStaticMeshComponent> m_pBeaconArrowComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    //StatusTextCompの操作に使用する参照です。
    TObjectPtr<UTextRenderComponent> m_pStatusTextComp;

    //ゴールの見た目を変化させるマテリアルインスタンス
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal|Magic Circle", meta = (DisplayName = "Magic Circle Niagara"))
    //MagicCircleSystemの操作に使用する参照です。
    TObjectPtr<UNiagaraSystem> m_pMagicCircleSystem;

    //ゴールの見た目を変化させるマテリアルインスタンスのパラメータ名
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal|Magic Circle")
    FName m_magicCircleColorParameter;

    //inactiveゴール色をゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal|Magic Circle")
    //inactiveゴール色をゲーム処理から参照できるように管理します。
    FLinearColor m_inactiveGoalColor;

    //activeゴール色をゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal|Magic Circle")
    //activeゴール色をゲーム処理から参照できるように管理します。
    FLinearColor m_activeGoalColor;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal|Magic Circle", meta = (ClampMin = "0.0"))
    //goalLight強さの調整値です。
    float m_goalLightIntensity;

    //ゴールを有効化してからクリアまでの遅延時間
    UPROPERTY(EditAnywhere, Category = "Goal", meta = (ClampMin = "0.0"))
    float m_clearDelay;

    //ゴールを有効化した後に遷移するレベル名
    UPROPERTY(EditAnywhere, Category = "Goal")
    FName m_gameClearLevelName;

    //ゴールに到達した時に表示するUIウィジェットのクラス
    UPROPERTY(EditAnywhere, Category = "Goal|UI")
    TSubclassOf<UUserWidget> m_goalWidgetClass;

  private:
    //コンポネントの参照を解決する関数
    bool ResolveComponents();

    //ゴールの有効化状態を反映する関数
    void ApplyGoalVisualState(bool _bActive);

    //ゴールの有効化状態を反映する関数
    void ApplyColorToMeshMaterial(const FLinearColor& _goalColor);

  private:
    //ゴールの見た目を変化させるマテリアルインスタンス
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> m_pDynamicGoalMaterial;

    //ゴールが有効化されているかどうかを追跡するフラグ
    bool m_bActivated;
    //TransitionRequestedかを示します。
    bool m_bTransitionRequested;
    //visualTimeを秒単位で指定します。
    float m_visualTime;
    //clearTimerを秒単位で指定します。
    FTimerHandle m_clearTimer;
};
