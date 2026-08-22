#include "FinalBossChara.h"

//コンストラクタ
AFinalBossChara::AFinalBossChara()
{
    m_meleeRange = 300.0f;
    m_chargeDistance = 850.0f;
}

//ボスのAIアーキタイプを取得する関数
EBossAIArchetype AFinalBossChara::GetBossAIArchetype() const { return EBossAIArchetype::FinalBoss; }

//ユーティリティAIのスコアをボス種別ごとに微調整する関数
float AFinalBossChara::ModifyUtilityScore(EBossTacticalAction _action, const FBossDecisionContext& _context, float _baseScore) const
{
    //基本スコアを初期化
    float modifiedScore = _baseScore;

    //アクションに応じてスコアを微調整
    switch (_action)
    {
    case EBossTacticalAction::LightCombo: modifiedScore += 0.20f; break;

    case EBossTacticalAction::PowerSlam: modifiedScore += 0.20f + (_context.bPlayerMovingTowardBoss ? 0.18f : 0.0f); break;

    case EBossTacticalAction::ChargeRush:
        modifiedScore +=
            0.10f + ((_context.bPlayerHealing || _context.bPlayerReloading) ? 0.34f : 0.0f) + (_context.bPlayerMovingAwayFromBoss ? 0.16f : 0.0f);
        break;

    case EBossTacticalAction::BackStep: modifiedScore += 0.08f + (_context.RecentBossDamagePressure * 0.22f); break;

    case EBossTacticalAction::CircleLeft:
    case EBossTacticalAction::CircleRight: modifiedScore += _context.bPlayerAiming ? 0.20f : 0.04f; break;

    case EBossTacticalAction::Retreat:
        //「GetHealthRatio() < 0.25f」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
        if (GetHealthRatio() < 0.25f)
        {
            modifiedScore += 0.10f;
        }
        break;

    default: break;
    }

    //modifiedScoreは、ゲーム判定に使用する数値を計算し、後続の比較または更新へ渡すために使います。
    return modifiedScore;
}
