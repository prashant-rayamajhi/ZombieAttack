#include "CombatCrosshairWidget.h"

#include "Rendering/DrawElements.h"

//Aimingをゲーム内の対象へ反映します。
void UCombatCrosshairWidget::SetAiming(bool _bAiming)
{
    m_bAiming = _bAiming;
    Invalidate(EInvalidateWidgetReason::Paint);
}

//敵を倒した瞬間にクロスヘアを赤く点滅させます。
void UCombatCrosshairWidget::FlashKillConfirmation()
{
    m_killFlashRemaining = 0.38f;
    Invalidate(EInvalidateWidgetReason::Paint);
}

//Widgetのアニメーションと表示値をフレームごとに更新します。
void UCombatCrosshairWidget::NativeTick(const FGeometry& _geometry, float _deltaTime)
{
    //Widgetのアニメーションと表示値をフレームごとに更新します。
    Super::NativeTick(_geometry, _deltaTime);
    if (m_killFlashRemaining > 0.0f)
    {
        m_killFlashRemaining = FMath::Max(0.0f, m_killFlashRemaining - _deltaTime);
        Invalidate(EInvalidateWidgetReason::Paint);
    }
}

//Slateの描画領域へ照準または案内表示を描きます。
int32 UCombatCrosshairWidget::NativePaint(const FPaintArgs& _args, const FGeometry& _geometry, const FSlateRect& _cullingRect,
                                          FSlateWindowElementList& _drawElements, int32 _layerId, const FWidgetStyle& _widgetStyle,
                                          bool _bParentEnabled) const
{
    const int32 baseLayer = Super::NativePaint(_args, _geometry, _cullingRect, _drawElements, _layerId, _widgetStyle, _bParentEnabled);
    const FVector2D center = _geometry.GetLocalSize() * 0.5f;
    //撃破確認用の赤いクロスヘアを表示中かを示します。
    const bool bKillConfirmed = m_killFlashRemaining > 0.0f;
    const float crosshairGap = m_bAiming ? 5.0f : 9.0f;
    const float armLength = 7.0f;
    const float thickness = bKillConfirmed ? 2.8f : 1.8f;
    //色を保持します。
    const FLinearColor color = bKillConfirmed ? FLinearColor(1.0f, 0.035f, 0.02f, 1.0f) : FLinearColor(0.78f, 0.88f, 0.82f, 0.92f);

    const auto drawLine = [&_geometry, &_drawElements, baseLayer, color, thickness](const FVector2D& _start, const FVector2D& _end)
    {
        TArray<FVector2D> points;
        points.Add(_start);
        points.Add(_end);
        FSlateDrawElement::MakeLines(_drawElements, baseLayer + 1, _geometry.ToPaintGeometry(), points, ESlateDrawEffect::None, color, true,
                                     thickness);
    };

    drawLine(center + FVector2D(-crosshairGap - armLength, 0.0f), center + FVector2D(-crosshairGap, 0.0f));
    drawLine(center + FVector2D(crosshairGap, 0.0f), center + FVector2D(crosshairGap + armLength, 0.0f));
    drawLine(center + FVector2D(0.0f, -crosshairGap - armLength), center + FVector2D(0.0f, -crosshairGap));
    drawLine(center + FVector2D(0.0f, crosshairGap), center + FVector2D(0.0f, crosshairGap + armLength));
    if (bKillConfirmed)
    {
        const float diagonal = 7.0f;
        drawLine(center + FVector2D(-diagonal, -diagonal), center + FVector2D(diagonal, diagonal));
        drawLine(center + FVector2D(diagonal, -diagonal), center + FVector2D(-diagonal, diagonal));
    }

    return baseLayer + 1;
}
