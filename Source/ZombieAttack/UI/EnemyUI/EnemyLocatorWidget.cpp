#include "EnemyLocatorWidget.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "ZombieAttack/Enemy/EnemyChara.h"

//Slateの描画領域へ照準または案内表示を描きます。
int32 UEnemyLocatorWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                                       FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
                                       bool bParentEnabled) const
{
    //parentLayerは、Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElemen…から算出した数値を後続の判定または計算に使います。
    const int32 parentLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

    //プレイヤーコントローラーを保持します。
    APlayerController* playerController = GetOwningPlayer();
    //playerPawnは、playerController ? playerController->GetPawn() : nullptrから取得した参照を後続の呼び出しで使います。
    APawn* playerPawn = playerController ? playerController->GetPawn() : nullptr;
    //ワールドを返します。
    UWorld* world = GetWorld();
    //「!playerController || !playerPawn || !world」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (!playerController || !playerPawn || !world) { return parentLayer; }

    //nearestAlertEnemyは、nullptrから取得した参照を後続の呼び出しで使います。
    const AEnemyChara* nearestAlertEnemy = nullptr;
    //nearestDistanceSquaredは、TNumericLimits<float>::Max()から算出した数値を後続の判定または計算に使います。
    float nearestDistanceSquared = TNumericLimits<float>::Max();
    //「TActorIterator<AEnemyChara> iterator(world); iterator; ++iterator」で列挙される各要素へ、ループ本体の判定と更新を適用します。
    for (TActorIterator<AEnemyChara> iterator(world); iterator; ++iterator)
    {
        //敵を保持します。
        const AEnemyChara* enemy = *iterator;
        //「!IsValid(enemy) || enemy->IsDead() || !enemy->IsRedOutlineEnabled()」が成立するとき、現在の要素を飛ばして次の要素へ進みます。
        if (!IsValid(enemy) || enemy->IsDead() || !enemy->IsRedOutlineEnabled())
        {
            continue;
        }

        //distanceSquaredは、FVector::DistSquared2D(playerPawn->GetActorLocation(), enemy->GetActorL…から算出した数値を後続の判定または計算に使います。
        const float distanceSquared = FVector::DistSquared2D(playerPawn->GetActorLocation(), enemy->GetActorLocation());
        //「distanceSquared < nearestDistanceSquared」が成立するとき、nearestDistanceSquaredを更新します。
        if (distanceSquared < nearestDistanceSquared)
        {
            nearestDistanceSquared = distanceSquared;
            nearestAlertEnemy = enemy;
        }
    }
    //「!nearestAlertEnemy」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (!nearestAlertEnemy) { return parentLayer; }

    //カメラ位置を保持します。
    FVector cameraLocation;
    //カメラ回転を保持します。
    FRotator cameraRotation;
    playerController->GetPlayerViewPoint(cameraLocation, cameraRotation);
    //directionToEnemyは、nearestAlertEnemy->GetActorLocation() - cameraLocationから取得した参照を後続の呼び出しで使います。
    FVector directionToEnemy = nearestAlertEnemy->GetActorLocation() - cameraLocation;
    directionToEnemy.Z = 0.0f;
    //「!directionToEnemy.Normalize()」が成立するとき、DegreesToRadiansを呼び出します。
    if (!directionToEnemy.Normalize()) { return parentLayer; }

    //relativeYawは、FMath::DegreesToRadians(FMath::FindDeltaAngleDegrees(cameraRotation.Yaw…から算出した数値を後続の判定または計算に使います。
    const float relativeYaw = FMath::DegreesToRadians(FMath::FindDeltaAngleDegrees(cameraRotation.Yaw, directionToEnemy.Rotation().Yaw));
    //screenDirectionは、screenDirectionの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    FVector2D screenDirection(FMath::Sin(relativeYaw), -FMath::Cos(relativeYaw));
    screenDirection.Normalize();

    //localSizeは、AllottedGeometry.GetLocalSize()から求めた空間情報を位置または向きの計算に使います。
    const FVector2D localSize = AllottedGeometry.GetLocalSize();
    //centerは、localSize * 0.5fから求めた空間情報を位置または向きの計算に使います。
    const FVector2D center = localSize * 0.5f;
    //safeHalfExtentは、safeHalfExtentの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    const FVector2D safeHalfExtent(FMath::Max(20.0f, center.X - 58.0f), FMath::Max(20.0f, center.Y - 58.0f));
    const float horizontalScale = FMath::Abs(screenDirection.X) > KINDA_SMALL_NUMBER
                                      //入力値の大きさを符号なしで求めます。
                                      ? safeHalfExtent.X / FMath::Abs(screenDirection.X)
                                      : TNumericLimits<float>::Max();
    const float verticalScale = FMath::Abs(screenDirection.Y) > KINDA_SMALL_NUMBER
                                    //入力値の大きさを符号なしで求めます。
                                    ? safeHalfExtent.Y / FMath::Abs(screenDirection.Y)
                                    : TNumericLimits<float>::Max();
    //arrowCenterは、center + screenDirection * FMath::Min(horizontalScale, verticalScale)から求めた空間情報を位置または向きの計算に使います。
    const FVector2D arrowCenter = center + screenDirection * FMath::Min(horizontalScale, verticalScale);

    //perpendicularは、perpendicularの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    const FVector2D perpendicular(-screenDirection.Y, screenDirection.X);
    //tipは、arrowCenter + screenDirection * 17.0fから求めた空間情報を位置または向きの計算に使います。
    const FVector2D tip = arrowCenter + screenDirection * 17.0f;
    //baseは、arrowCenter - screenDirection * 12.0fの成立可否を後続の分岐で判定するために使います。
    const FVector2D base = arrowCenter - screenDirection * 12.0f;
    //arrowPointsは、位置と向きの計算結果を移動、照準、または描画位置へ反映するために使います。
    TArray<FVector2D> arrowPoints;
    arrowPoints.Reserve(4);
    arrowPoints.Add(tip);
    arrowPoints.Add(base + perpendicular * 10.0f);
    arrowPoints.Add(base - perpendicular * 10.0f);
    arrowPoints.Add(tip);

    //pulseは、0.72f + 0.28f * FMath::Sin(world->GetTimeSeconds() * 5.0f)から算出した数値を後続の判定または計算に使います。
    const float pulse = 0.72f + 0.28f * FMath::Sin(world->GetTimeSeconds() * 5.0f);
    FSlateDrawElement::MakeLines(OutDrawElements, parentLayer + 1, AllottedGeometry.ToPaintGeometry(), arrowPoints, ESlateDrawEffect::None,
                                 FLinearColor(1.0f, 0.015f, 0.01f, pulse), true, 4.0f);

    return parentLayer + 1;
}
