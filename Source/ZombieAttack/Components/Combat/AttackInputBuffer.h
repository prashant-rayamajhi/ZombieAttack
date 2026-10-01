#pragma once
#include "CoreMinimal.h"

//操作終了直前の一入力だけを受け付け、連打を後からまとめて発射しない予約。
struct FAttackInputBuffer
{
    //押した時点の装備番号。別の武器へ持ち越して誤射しないために使う。
    int32 m_slot = INDEX_NONE;
    //入力を受け付ける期限。長いリロード中の入力は完了前に捨てる。
    double m_expires = 0.0;
    //最後に押した一回で予約を置き換える。
    void Queue(int32 _slot, double _now) { m_slot = _slot; m_expires = _now + 0.18; }
    //死亡、操作禁止、入力解除で予約を破棄する。
    void Clear() { m_slot = INDEX_NONE; }
    //同じ武器で期限内の入力だけを一度取り出す。
    bool Consume(int32 _slot, double _now)
    {
        const bool valid = m_slot != INDEX_NONE && m_slot == _slot && _now <= m_expires;
        Clear();
        return valid;
    }
};
