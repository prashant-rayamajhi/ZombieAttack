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

    //「m_killFlashRemaining > 0.0f」が成立するとき、Maxを呼び出します。
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
    //baseLayerは、Super::NativePaint(_args, _geometry, _cullingRect, _drawElements, _laye…の成立可否を後続の分岐で判定するために使います。
    const int32 baseLayer = Super::NativePaint(_args, _geometry, _cullingRect, _drawElements, _layerId, _widgetStyle, _bParentEnabled);

    //centerは、_geometry.GetLocalSize() * 0.5fから求めた空間情報を位置または向きの計算に使います。
    const FVector2D center = _geometry.GetLocalSize() * 0.5f;
    //撃破確認用の赤いクロスヘアを表示中かを示します。
    const bool bKillConfirmed = m_killFlashRemaining > 0.0f;
    //gapは、m_bAiming ? 5.0f : 9.0fから算出した数値を後続の判定または計算に使います。
    const float gap = m_bAiming ? 5.0f : 9.0f;
    //armLengthは、bKillConfirmed ? 13.0f : (m_bAiming ? 9.0f : 11.0f)から算出した数値を後続の判定または計算に使います。
    const float armLength = bKillConfirmed ? 13.0f : (m_bAiming ? 9.0f : 11.0f);
    //thicknessは、bKillConfirmed ? 2.8f : 1.8fから算出した数値を後続の判定または計算に使います。
    const float thickness = bKillConfirmed ? 2.8f : 1.8f;
    //色を保持します。
    const FLinearColor color = bKillConfirmed ? FLinearColor(1.0f, 0.035f, 0.02f, 1.0f) : FLinearColor(0.78f, 0.88f, 0.82f, 0.92f);

    const auto drawLine = [&_geometry, &_drawElements, baseLayer, color, thickness](const FVector2D& _start, const FVector2D& _end)
    {
        //pointsは、位置と向きの計算結果を移動、照準、または描画位置へ反映するために使います。
        TArray<FVector2D> points;
        points.Add(_start);
        points.Add(_end);
        FSlateDrawElement::MakeLines(_drawElements, baseLayer + 1, _geometry.ToPaintGeometry(), points, ESlateDrawEffect::None, color, true,
                                     thickness);
    };

    drawLine(center + FVector2D(-gap - armLength, 0.0f), center + FVector2D(-gap, 0.0f));
    drawLine(center + FVector2D(gap, 0.0f), center + FVector2D(gap + armLength, 0.0f));
    drawLine(center + FVector2D(0.0f, -gap - armLength), center + FVector2D(0.0f, -gap));
    drawLine(center + FVector2D(0.0f, gap), center + FVector2D(0.0f, gap + armLength));

    //「bKillConfirmed」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
    if (bKillConfirmed)
    {
        //diagonalは、7.0fから算出した数値を後続の判定または計算に使います。
        const float diagonal = 7.0f;
        drawLine(center + FVector2D(-diagonal, -diagonal), center + FVector2D(diagonal, diagonal));
        drawLine(center + FVector2D(diagonal, -diagonal), center + FVector2D(-diagonal, diagonal));
    }

    return baseLayer + 1;
}
