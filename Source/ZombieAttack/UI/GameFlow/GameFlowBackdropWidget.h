#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFlowBackdropWidget.generated.h"

//森、霧、雨、脱出ゲートを図形で描き、画像素材なしで作品の舞台を伝える
UCLASS()
class ZOMBIEATTACK_API UGameFlowBackdropWidget : public UUserWidget
{
    GENERATED_BODY()

  public:
    //開始は冷色、クリアは朝焼け、敗北は赤い警告灯へ変える
    void SetScene(int32 _scene) { m_scene = _scene; }

  protected:
    //時間とマウス位置から霧、雨、奥行きの移動量を更新する
    virtual void NativeTick(const FGeometry& _geometry, float _deltaTime) override;
    //背景専用の描画層へ森と灯りを描く
    virtual int32 NativePaint(const FPaintArgs& _args, const FGeometry& _geometry, const FSlateRect& _cullingRect,
                             FSlateWindowElementList& _elements, int32 _layer, const FWidgetStyle& _style, bool _bEnabled) const override;

  private:
    //画面種類ごとの空と灯りの配色
    int32 m_scene = 0;
    //雨や霧を滑らかに動かす経過秒数
    float m_time = 0.0f;
    //ポインターに対する緩やかな視差
    FVector2D m_parallax = FVector2D::ZeroVector;
};
