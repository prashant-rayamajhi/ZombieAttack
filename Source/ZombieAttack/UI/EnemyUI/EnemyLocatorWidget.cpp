#include "EnemyLocatorWidget.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "ZombieAttack/Enemy/EnemyChara.h"
#include "ZombieAttack/Goal/GoalActor.h"

//対象の種類と距離を併記し、画面外では向くべき方向を言葉でも示す。
int32 UEnemyLocatorWidget::NativePaint(const FPaintArgs& _args, const FGeometry& _geometry, const FSlateRect& _cullingRect,
    FSlateWindowElementList& _drawElements, int32 _layerId, const FWidgetStyle& _widgetStyle, bool _bParentEnabled) const
{
    const int32 layer = Super::NativePaint(_args, _geometry, _cullingRect, _drawElements, _layerId, _widgetStyle, _bParentEnabled);
    APlayerController* controller = GetOwningPlayer();
    APawn* pawn = controller ? controller->GetPawn() : nullptr;
    if (!pawn || !GetWorld()) { return layer; }

    //見失った敵のうち一体だけを案内し、マーカーの重なりで視界を塞がない。
    AActor* target = nullptr;
    float distanceSquared = TNumericLimits<float>::Max();
    bool extraction = false;
    for (TActorIterator<AEnemyChara> enemy(GetWorld()); enemy; ++enemy)
    {
        if (enemy->IsDead() || !enemy->IsRedOutlineEnabled()) { continue; }
        const float distance = FVector::DistSquared(pawn->GetActorLocation(), enemy->GetActorLocation());
        if (distance >= distanceSquared) { continue; }
        distanceSquared = distance;
        target = *enemy;
    }
    //敵を倒し終えた後は、同じ表示枠を出口案内へ切り替える。
    if (!target)
    {
        for (TActorIterator<AGoalActor> goal(GetWorld()); goal; ++goal)
        {
            if (!goal->IsActivated()) { continue; }
            target = *goal;
            extraction = true;
            distanceSquared = FVector::DistSquared(pawn->GetActorLocation(), goal->GetActorLocation());
            break;
        }
    }
    if (!target) { return layer; }

    FVector cameraLocation;
    FRotator cameraRotation;
    controller->GetPlayerViewPoint(cameraLocation, cameraRotation);
    const FVector offset = target->GetActorLocation() - cameraLocation;
    const float yaw = FMath::FindDeltaAngleDegrees(cameraRotation.Yaw, offset.Rotation().Yaw);
    const FVector2D size = _geometry.GetLocalSize();
    if (size.X < 400 || size.Y < 300) { return layer; }
    FVector2D point;
    const FVector focus = target->GetActorLocation() + FVector(0, 0, extraction ? 200 : 110);
    const bool projected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(controller, focus, point, true);
    const bool onScreen = projected && FMath::Abs(yaw) < 85 && point.X > 110 && point.X < size.X - 210 &&
        point.Y > 140 && point.Y < size.Y - 160;

    //背後の対象は下中央、左右の対象は画面端に固定し、上下に回る三角形を使わない。
    FString direction;
    if (!onScreen)
    {
        if (FMath::Abs(yaw) > 130)
        {
            point = FVector2D(size.X * 0.5f, size.Y - 155);
            direction = TEXT("TURN AROUND");
        }
        else
        {
            point = FVector2D(yaw < 0 ? 110 : size.X - 210, size.Y * 0.5f);
            direction = yaw < 0 ? TEXT("<< LOOK LEFT") : TEXT("LOOK RIGHT >>");
        }
    }
    const FLinearColor color = extraction ? FLinearColor(0.46f, 0.64f, 0.48f) : FLinearColor(0.72f, 0.29f, 0.18f);
    const FVector2D panel = point + FVector2D(18, -16);
    const FSlateBrush* brush = FCoreStyle::Get().GetBrush("WhiteBrush");
    FSlateDrawElement::MakeBox(_drawElements, layer + 1, _geometry.ToPaintGeometry(FVector2D(168, 62), FSlateLayoutTransform(panel)),
        brush, ESlateDrawEffect::None, FLinearColor(0.015f, 0.022f, 0.028f, 0.88f));
    //菱形は対象の位置を表し、矢印の向きと混同しないよう回転させない。
    TArray<FVector2D> diamond = {point + FVector2D(0, -9), point + FVector2D(9, 0), point + FVector2D(0, 9),
        point + FVector2D(-9, 0), point + FVector2D(0, -9)};
    FSlateDrawElement::MakeLines(_drawElements, layer + 2, _geometry.ToPaintGeometry(), diamond, ESlateDrawEffect::None, color, true, 2);
    const FString title = extraction ? TEXT("EXTRACTION") : TEXT("HOSTILE");
    const FString detail = FString::Printf(TEXT("%dm  %s"), FMath::RoundToInt(FMath::Sqrt(distanceSquared) / 100), *direction);
    const FSlateFontInfo titleFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
    const FSlateFontInfo detailFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
    FSlateDrawElement::MakeText(_drawElements, layer + 2, _geometry.ToPaintGeometry(FVector2D(150, 22),
        FSlateLayoutTransform(panel + FVector2D(10, 7))), title, titleFont, ESlateDrawEffect::None, color);
    FSlateDrawElement::MakeText(_drawElements, layer + 2, _geometry.ToPaintGeometry(FVector2D(150, 20),
        FSlateLayoutTransform(panel + FVector2D(10, 33))), detail, detailFont, ESlateDrawEffect::None, FLinearColor::White);
    return layer + 2;
}
