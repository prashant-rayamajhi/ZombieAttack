#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawnPoint.generated.h"

//前方宣言
class USceneComponent;
//UArrowComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UArrowComponent;

//敵生成Pointの動作をまとめたクラス
UCLASS(BlueprintType)
class ZOMBIEATTACK_API AEnemySpawnPoint : public AActor
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //コンストラクタ
    AEnemySpawnPoint();

    //ランダムな位置を取得する関数
    FVector GetRandomizedLocation() const;

    //スポーン時の回転を取得する関数
    FRotator GetSpawnRotation() const;

    //スポーン時の回転を取得する関数
    FName GetRequiredGroundActorTag() const { return m_requiredGroundActorTag; }

  protected:
    //ルートコンポーネント
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> m_root;

    //スポーン方向を示す矢印コンポーネント
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UArrowComponent> m_arrow;

    //ランダムな位置を生成する半径
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Point", meta = (ClampMin = "0.0"))
    float m_randomRadius;

    //スポーン時に矢印の回転を使用するかどうかを設定するプロパティ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Point")
    bool m_bUsePointRotation;

    //スポーン時に必要な地面のタグを設定するプロパティ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Point")
    FName m_requiredGroundActorTag;
};
