#include "GameFlowBackdropWidget.h"
#include "GameFlowScene.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void UGameFlowBackdropWidget::NativeTick(const FGeometry& _geometry, float _deltaTime)
{
    Super::NativeTick(_geometry, _deltaTime);
    m_time += _deltaTime;
    FVector2D target = FVector2D::ZeroVector;
    if (APlayerController* controller = GetOwningPlayer())
    {
        float mouseX = 0.0f;
        float mouseY = 0.0f;
        int32 width = 1;
        int32 height = 1;
        controller->GetViewportSize(width, height);
        if (controller->GetMousePosition(mouseX, mouseY))
        {
            target.X = FMath::Clamp(mouseX / FMath::Max(1, width) - 0.5f, -0.5f, 0.5f);
            target.Y = FMath::Clamp(mouseY / FMath::Max(1, height) - 0.5f, -0.5f, 0.5f);
        }
    }
    m_parallax = FMath::Vector2DInterpTo(m_parallax, target, _deltaTime, 2.0f);
    InvalidateLayoutAndVolatility();
}

int32 UGameFlowBackdropWidget::NativePaint(const FPaintArgs& _args, const FGeometry& _geometry, const FSlateRect& _cullingRect,
                                          FSlateWindowElementList& _elements, int32 _layer, const FWidgetStyle& _style, bool _bEnabled) const
{
    const FVector2D size = _geometry.GetLocalSize();
    const FVector2D scale(size.X / 1920.0f, size.Y / 1080.0f);
    const FSlateBrush* brush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    const auto box = [&](FVector2D _position, FVector2D _size, FLinearColor _color, int32 _depth)
    {
        FSlateDrawElement::MakeBox(_elements, _layer + _depth,
            _geometry.ToPaintGeometry(_size * scale, FSlateLayoutTransform(_position * scale)), brush, ESlateDrawEffect::None, _color);
    };
    const auto line = [&](FVector2D _from, FVector2D _to, FLinearColor _color, float _width, int32 _depth)
    {
        TArray<FVector2D> points = {_from * scale, _to * scale};
        FSlateDrawElement::MakeLines(_elements, _layer + _depth, _geometry.ToPaintGeometry(), points,
                                    ESlateDrawEffect::None, _color, true, _width * scale.X);
    };
    //実景がある場合は文字の背後だけ暗くし、3Dの森を隠さない。
    for (TActorIterator<AGameFlowScene> scene(GetWorld()); scene; ++scene)
    {
        for (int32 strip = 0; strip < 28; ++strip)
        {
            box(FVector2D(strip * 40, 0), FVector2D(41, 1080), FLinearColor(0.005f, 0.008f, 0.011f, 0.94f - strip * 0.031f), 1);
        }
        return _layer + 1;
    }
    //空の明るさは右側だけに残し、左の操作メニューを読みやすくする
    const FLinearColor sky = m_scene == 1 ? FLinearColor(0.11f, 0.15f, 0.14f) : FLinearColor(0.025f, 0.055f, 0.064f);
    box(FVector2D::ZeroVector, FVector2D(1920, 1080), FLinearColor(0.008f, 0.015f, 0.02f), 0);
    for (int32 strip = 0; strip < 48; ++strip)
    {
        const float light = FMath::Sin(static_cast<float>(strip) / 48.0f * PI);
        box(FVector2D(0, strip * 22.5f), FVector2D(1920, 23), sky * (0.35f + light * 0.65f), 1);
    }
    //三層の樹木を違う量だけ動かし、静止画ではない森の奥行きを出す
    for (int32 depth = 0; depth < 3; ++depth)
    {
        FRandomStream trees(241 + depth);
        const float offset = m_parallax.X * (12.0f + depth * 24.0f);
        const FLinearColor trunk = FLinearColor(0.014f, 0.03f, 0.031f) * (1.5f - depth * 0.45f);
        for (int32 tree = 0; tree < 19; ++tree)
        {
            const float x = tree * 115.0f + trees.FRandRange(-65.0f, 65.0f) + offset;
            const float base = 800.0f + depth * 110.0f;
            const float top = trees.FRandRange(10.0f, 300.0f);
            const float width = trees.FRandRange(9.0f, 19.0f) + depth * 8.0f;
            line(FVector2D(x, base), FVector2D(x + 20.0f, top), trunk, width, 2 + depth * 2);
            for (int32 branch = 0; branch < 9; ++branch)
            {
                const float y = top + branch * 52.0f + trees.FRandRange(-18.0f, 18.0f);
                const float reach = 24.0f + branch * trees.FRandRange(7.0f, 14.0f);
                const float leftTip = y + trees.FRandRange(18.0f, 60.0f);
                const float rightTip = y + trees.FRandRange(25.0f, 80.0f);
                line(FVector2D(x + 15.0f, y), FVector2D(x - reach, leftTip), trunk, width * 0.65f, 2 + depth * 2);
                line(FVector2D(x + 15.0f, y + 7), FVector2D(x + reach * 0.85f, rightTip), trunk, width * 0.55f, 2 + depth * 2);
            }
        }
        //幅の異なる薄い霧を重ね、明滅を使わず穏やかに流す
        for (int32 fog = 0; fog < 9; ++fog)
        {
            const float y = 540.0f + fog * 36.0f + FMath::Sin(m_time * 0.14f + fog + depth) * 22.0f;
            line(FVector2D(0, y), FVector2D(1920, y + 45), FLinearColor(0.21f, 0.28f, 0.27f, 0.018f), 32, 3 + depth * 2);
        }
    }
    //右手の脱出ゲートを目印にする。敗北画面では警告灯を赤くする
    const float gateX = 1360.0f + m_parallax.X * 32.0f;
    const FLinearColor steel(0.18f, 0.23f, 0.22f);
    const FLinearColor lamp = m_scene == 2 ? FLinearColor(0.68f, 0.10f, 0.06f) : FLinearColor(0.64f, 0.61f, 0.40f);
    line(FVector2D(gateX - 130, 860), FVector2D(gateX - 130, 465), steel, 11, 8);
    line(FVector2D(gateX + 130, 860), FVector2D(gateX + 130, 465), steel, 11, 8);
    line(FVector2D(gateX - 130, 465), FVector2D(gateX + 130, 465), steel, 11, 8);
    for (int32 bar = -5; bar <= 5; ++bar)
    {
        //クリア時は左右の扉を外へ開き、中央の通路を空ける
        if (m_scene == 1 && bar == 0) { continue; }
        const float direction = FMath::Sign(static_cast<float>(bar));
        const float distance = FMath::Abs(static_cast<float>(bar));
        const float x = m_scene == 1 ? gateX + direction * (130.0f + distance * 14.0f) : gateX + bar * 23.0f;
        const float top = m_scene == 1 ? 490.0f + distance * 9.0f : 490.0f;
        const float bottom = m_scene == 1 ? 860.0f - distance * 8.0f : 860.0f;
        line(FVector2D(x, top), FVector2D(x, bottom), steel * 0.6f, 3, 8);
    }
    for (int32 ray = -25; ray <= 25; ++ray)
    {
        FLinearColor glow = lamp;
        glow.A = 0.012f;
        line(FVector2D(gateX, 452), FVector2D(gateX + ray * 11.0f + FMath::Sin(m_time * 0.19f) * 30, 925), glow, 12, 9);
    }
    box(FVector2D(gateX - 22, 445), FVector2D(44, 7), lamp, 10);
    //雨は一定の種から位置を決め、毎フレームちらつく乱数を使わない
    if (m_scene != 1)
    {
        FRandomStream rain(731);
        for (int32 drop = 0; drop < 65; ++drop)
        {
            const float x = rain.FRandRange(0, 1920);
            const float y = FMath::Fmod(rain.FRandRange(0, 1080) + m_time * 220.0f, 1080.0f);
            line(FVector2D(x, y), FVector2D(x - 6, y + 25), FLinearColor(0.42f, 0.55f, 0.57f, 0.14f), 1, 11);
        }
    }
    //メニュー側を段階的に暗くして、背景の線が文字へ重ならないようにする
    for (int32 strip = 0; strip < 28; ++strip)
    {
        box(FVector2D(strip * 40, 0), FVector2D(41, 1080), FLinearColor(0.005f, 0.008f, 0.011f, 0.94f - strip * 0.031f), 12);
    }
    return _layer + 12;
}
