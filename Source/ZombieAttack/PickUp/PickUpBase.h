#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickUpBase.generated.h"

//USphereComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class USphereComponent;
//UStaticMeshComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UStaticMeshComponent;
//UBillboardComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UBillboardComponent;
//UPointLightComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UPointLightComponent;
//UTexture2Dは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UTexture2D;
//APlayerCharaは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class APlayerChara;

//アイテム種類の動作をまとめたクラス
UENUM(BlueprintType)
enum class EItemType : uint8
{
    //プレイヤーの体力を回復する
    EIT_Health UMETA(DisplayName = "Health"),
    //ハンドガンの予備弾薬を補充する
    EIT_Ammo UMETA(DisplayName = "Pistol Ammo"),
    //ARの予備弾薬を補充する
    EIT_ARAmmo UMETA(DisplayName = "AR Ammo"),
    //ARを使用可能にする
    EIT_WeaponAR UMETA(DisplayName = "AR Weapon Unlock"),
    //上記以外の拾得物に使用する
    EIT_Other UMETA(DisplayName = "Other")
};

//PickUpBaseの動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API APickUpBase : public AActor
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    APickUpBase();

    //ゲーム開始時の初期設定を行います。
    virtual void BeginPlay() override;
    //毎フレームの更新を行います。
    virtual void Tick(float DeltaTime) override;

    //アイテムを取得し、効果を反映します。
    UFUNCTION(BlueprintCallable, Category = "Item")
    void PickUpItem(EItemType Type, float Value);

    //アイテム種類を返します。
    EItemType GetItemType() const { return m_itemType; }
    //HasPickupPresentationは、名前が示す条件の成立可否を呼び出し元へ返します。
    bool HasPickupPresentation() const;

  protected:
    //MeshCompの操作に使用する参照です。
    UPROPERTY(VisibleAnywhere, Category = "Components")
    //MeshCompの操作に使用する参照です。
    TObjectPtr<UStaticMeshComponent> m_pMeshComp;

    //SphereCompの操作に使用する参照です。
    UPROPERTY(VisibleAnywhere, Category = "Components")
    //SphereCompの操作に使用する参照です。
    TObjectPtr<USphereComponent> m_pSphereComp;

    //ItemIconCompの操作に使用する参照です。
    UPROPERTY(VisibleAnywhere, Category = "Components")
    //ItemIconCompの操作に使用する参照です。
    TObjectPtr<UBillboardComponent> m_pItemIconComp;

    //ItemLightCompの操作に使用する参照です。
    UPROPERTY(VisibleAnywhere, Category = "Components")
    //ItemLightCompの操作に使用する参照です。
    TObjectPtr<UPointLightComponent> m_pItemLightComp;

    //アイテム種類を保持します。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EItemType m_itemType;

    //アイテム値を保持します。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    float m_itemValue;

    //体力メッシュを保持します。
    UPROPERTY(EditAnywhere, Category = "Item|Mesh")
    TObjectPtr<UStaticMesh> m_pHealthMesh;

    //弾薬メッシュを保持します。
    UPROPERTY(EditAnywhere, Category = "Item|Mesh")
    TObjectPtr<UStaticMesh> m_pAmmoMesh;

    //AR弾薬Meshの操作に使用する参照です。
    UPROPERTY(EditAnywhere, Category = "Item|Mesh")
    //AR弾薬Meshの操作に使用する参照です。
    TObjectPtr<UStaticMesh> m_pARAmmoMesh;

    //ARWeaponMeshの操作に使用する参照です。
    UPROPERTY(EditAnywhere, Category = "Item|Mesh")
    //ARWeaponMeshの操作に使用する参照です。
    TObjectPtr<UStaticMesh> m_pARWeaponMesh;

    //体力Iconの操作に使用する参照です。
    UPROPERTY(VisibleDefaultsOnly, Category = "Item|Presentation")
    //体力Iconの操作に使用する参照です。
    TObjectPtr<UTexture2D> m_pHealthIcon;

    //弾薬Iconの操作に使用する参照です。
    UPROPERTY(VisibleDefaultsOnly, Category = "Item|Presentation")
    //弾薬Iconの操作に使用する参照です。
    TObjectPtr<UTexture2D> m_pAmmoIcon;

    //floatHeightをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Item|Float")
    //floatHeightをゲーム処理から参照できるように管理します。
    float m_floatHeight;

    //AR本体だけを通常のドロップより低く浮かせる高さ補正です。
    UPROPERTY(EditAnywhere, Category = "Item|Float")
    float m_arWeaponFloatHeightAdjustment;

    //floatAmplitudeをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Item|Float")
    //floatAmplitudeをゲーム処理から参照できるように管理します。
    float m_floatAmplitude;

    //float速度の調整値です。
    UPROPERTY(EditAnywhere, Category = "Item|Float")
    //float速度の調整値です。
    float m_floatSpeed;

    //rotate速度の調整値です。
    UPROPERTY(EditAnywhere, Category = "Item|Float")
    //rotate速度の調整値です。
    float m_rotateSpeed;

    //OverlapBeginが発生したときの処理を行います。
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex,
                        bool bFromSweep, const FHitResult& SweepResult);

  private:
    //ApplyMeshByTypeは、引数の内容をゲーム中の状態または表示へ反映します。
    void ApplyMeshByType();

  private:
    //生成位置を保持します。
    FVector m_spawnLocation;
    //floatTimeを秒単位で指定します。
    float m_floatTime;
    //Consumedかを示します。
    bool m_bConsumed;
};
