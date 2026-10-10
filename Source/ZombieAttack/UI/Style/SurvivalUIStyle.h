#pragma once

#include "CoreMinimal.h"

class UWidgetTree;

//森林の画面に馴染む共通色と文字組み。警告以外では強い赤や発光色を使わない。
namespace SurvivalUI
{
    //霧の上でも読める暖色寄りの白。
    inline const FLinearColor Text(0.86f, 0.84f, 0.77f);
    //選択中の項目と予備弾数に使うくすんだ黄土色。
    inline const FLinearColor Accent(0.58f, 0.48f, 0.29f);
    //補足情報を主表示より一段弱くする灰緑色。
    inline const FLinearColor Muted(0.43f, 0.48f, 0.43f);
    //体力に使う低彩度の緑。赤は被弾・瀕死・撃破通知だけに残す。
    inline const FLinearColor Health(0.27f, 0.48f, 0.32f);
    //背景の森を完全には隠さない暗いパネル。
    inline const FLinearColor Panel(0.009f, 0.013f, 0.011f, 0.76f);

    //C++生成と既存Widget BPの両方へ、共通の余白・見出し・体力バーを適用する。
    void Apply(UWidgetTree* _tree);
}
