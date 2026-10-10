#include "AmmoRadialWidget.h"
#include "../Style/SurvivalUIStyle.h"

#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

//SAmmoRadialは、この型にまとめられたゲーム機能と状態を管理します。
class SAmmoRadial : public SLeafWidget
{
  public:
    SLATE_BEGIN_ARGS(SAmmoRadial) {}
    SLATE_END_ARGS()

    void Construct(const FArguments&) {}

    void SetValues(float _clipPercent, float _reservePercent, const FLinearColor& _clipColor, const FLinearColor& _reserveColor,
                   const FLinearColor& _emptyColor, float _ringThickness)
    {
        m_clipPercent = FMath::Clamp(_clipPercent, 0.0f, 1.0f);
        m_reservePercent = FMath::Clamp(_reservePercent, 0.0f, 1.0f);
        m_clipColor = _clipColor;
        m_reserveColor = _reserveColor;
        m_emptyColor = _emptyColor;
        m_ringThickness = FMath::Max(1.0f, _ringThickness);
        Invalidate(EInvalidateWidgetReason::Paint);
    }

    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(180.0f, 180.0f); }

    virtual int32 OnPaint(const FPaintArgs& _args, const FGeometry& _allottedGeometry, const FSlateRect& _cullingRect,
                          FSlateWindowElementList& _outDrawElements, int32 _layerID, const FWidgetStyle& _widgetStyle,
                          bool _bParentEnabled) const override
    {
        const FVector2D localSize = _allottedGeometry.GetLocalSize();
        const FVector2D center = localSize * 0.5f;
        const float shortestSide = FMath::Min(localSize.X, localSize.Y);
        const float outerRadius = shortestSide * 0.44f;
        const float innerRadius = shortestSide * 0.30f;
        const ESlateDrawEffect drawEffects = _bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

        DrawArc(_allottedGeometry, _outDrawElements, _layerID, center, outerRadius, 1.0f, m_emptyColor, drawEffects);
        DrawArc(_allottedGeometry, _outDrawElements, _layerID + 1, center, innerRadius, 1.0f, m_emptyColor, drawEffects);
        DrawArc(_allottedGeometry, _outDrawElements, _layerID + 2, center, outerRadius, m_reservePercent, m_reserveColor, drawEffects);
        DrawArc(_allottedGeometry, _outDrawElements, _layerID + 3, center, innerRadius, m_clipPercent, m_clipColor, drawEffects);

        return _layerID + 3;
    }

  private:
    void DrawArc(const FGeometry& _geometry, FSlateWindowElementList& _outDrawElements, int32 _layerID, const FVector2D& _center, float _radius,
                 float _percent, const FLinearColor& _color, ESlateDrawEffect _drawEffects) const
    {
        constexpr int32 SegmentCount = 64;
        //残弾0で短い弧が残ると一発あるように見えるため、空のゲージは描かない。
        if (_percent <= 0.0f) { return; }
        const int32 activeSegments = FMath::Max(1, FMath::CeilToInt(SegmentCount * FMath::Clamp(_percent, 0.0f, 1.0f)));
        TArray<FVector2D> points;
        points.Reserve(activeSegments + 1);

        //残量に対応する角度まで円弧をつなぎ、空の部分は下地だけを残す。
        for (int32 index = 0; index <= activeSegments; ++index)
        {
            const float normalized = static_cast<float>(index) / static_cast<float>(SegmentCount);
            const float angle = -HALF_PI + normalized * TWO_PI;
            points.Add(_center + FVector2D(FMath::Cos(angle), FMath::Sin(angle)) * _radius);
        }

        FSlateDrawElement::MakeLines(_outDrawElements, _layerID, _geometry.ToPaintGeometry(), points, _drawEffects, _color, true, m_ringThickness);
    }

  private:
    float m_clipPercent = 1.0f;
    float m_reservePercent = 1.0f;
    float m_ringThickness = 7.0f;
    FLinearColor m_clipColor = FLinearColor::White;
    FLinearColor m_reserveColor = FLinearColor(0.72f, 0.04f, 0.03f, 1.0f);
    FLinearColor m_emptyColor = FLinearColor(0.08f, 0.08f, 0.08f, 0.75f);
};

//UAmmoRadialWidgetが使用するComponentと初期パラメータを設定します。
UAmmoRadialWidget::UAmmoRadialWidget()
    : m_clipColor(FLinearColor::White), m_reserveColor(0.72f, 0.04f, 0.03f, 1.0f), m_emptyColor(0.08f, 0.08f, 0.08f, 0.75f), m_ringThickness(7.0f),
      m_clipPercent(1.0f), m_reservePercent(1.0f)
{
}

//弾薬Percentをゲーム内の対象へ反映します。
void UAmmoRadialWidget::SetAmmoPercent(float _clipPercent, float _reservePercent)
{
    m_clipPercent = FMath::Clamp(_clipPercent, 0.0f, 1.0f);
    m_reservePercent = FMath::Clamp(_reservePercent, 0.0f, 1.0f);
    SynchronizeProperties();
}

//Widgetを現在のデータから組み直します。
TSharedRef<SWidget> UAmmoRadialWidget::RebuildWidget()
{
    SAssignNew(m_pSlateWidget, SAmmoRadial);
    return m_pSlateWidget.ToSharedRef();
}

//Slate Widgetの参照を解放し、再生成時に古い描画を残さないようにします。
void UAmmoRadialWidget::ReleaseSlateResources(bool _bReleaseChildren)
{
    Super::ReleaseSlateResources(_bReleaseChildren);
    m_pSlateWidget.Reset();
}

//Propertiesを実際の移動と同期させます。
void UAmmoRadialWidget::SynchronizeProperties()
{
    //Propertiesを実際の移動と同期させます。
    Super::SynchronizeProperties();
    if (m_pSlateWidget.IsValid())
    {
        m_pSlateWidget->SetValues(m_clipPercent, m_reservePercent, SurvivalUI::Text, SurvivalUI::Accent, m_emptyColor, 3.0f);
    }
}
