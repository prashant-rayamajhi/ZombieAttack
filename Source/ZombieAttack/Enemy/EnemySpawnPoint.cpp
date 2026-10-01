#include "EnemySpawnPoint.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

//コンストラクタ
AEnemySpawnPoint::AEnemySpawnPoint()
    : m_root(nullptr), m_arrow(nullptr), m_randomRadius(80.f), m_bUsePointRotation(true), m_requiredGroundActorTag(NAME_None)
{
    //Tickを無効化
    PrimaryActorTick.bCanEverTick = false;

    //ルートコンポーネントを作成
    m_root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = m_root;

    //矢印コンポーネントを作成して、スポーン方向を示す
    m_arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnDirection"));
    m_arrow->SetupAttachment(m_root);
    m_arrow->ArrowColor = FColor::Red;
    m_arrow->ArrowSize = 1.4f;
}

//ランダムな位置を取得する関数
FVector AEnemySpawnPoint::GetRandomizedLocation() const
{
    //ランダムな角度と半径を生成して、円形の範囲内でランダムな位置を計算する
    const float angle = FMath::FRandRange(0.f, 2.f * PI);
    //半径を保持します。
    const float radius = FMath::Sqrt(FMath::FRand()) * FMath::Max(0.f, m_randomRadius);
    //GetActorLocationは、呼び出し元が必要とする対象または計算結果を返します。
    return GetActorLocation() + FVector(FMath::Cos(angle) * radius, FMath::Sin(angle) * radius, 0.f);
}

//スポーン時の回転を取得する関数
FRotator AEnemySpawnPoint::GetSpawnRotation() const
{
    return m_bUsePointRotation ? GetActorRotation() : FRotator(0.f, FMath::FRandRange(-180.f, 180.f), 0.f);
}
