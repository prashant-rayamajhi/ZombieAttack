#include "MidBossChara.h"

//コンストラクタ
AMidBossChara::AMidBossChara()
{
    m_meleeRange = 250.0f;
    m_chargeDistance = 650.0f;
}

//ボスのAIアーキタイプを取得する関数
EBossAIArchetype AMidBossChara::GetBossAIArchetype() const { return EBossAIArchetype::MidBoss; }

//UtilityAIのスコアをボス種別ごとに微調整する関数
float AMidBossChara::ModifyUtilityScore(EBossTacticalAction _action, const FBossDecisionContext& _context, float _baseScore) const
{
    //基本スコアを初期化
    float modifiedScore = _baseScore;

    //アクションに応じてスコアを微調整
    switch (_action)
    {
    case EBossTacticalAction::LightCombo: modifiedScore += 0.15f + (_context.bPlayerReloading ? 0.28f : 0.0f); break;

    case EBossTacticalAction::PowerSlam: modifiedScore += 0.05f; break;

    case EBossTacticalAction::ChargeRush: modifiedScore += (_context.bPlayerHealing || _context.bPlayerReloading) ? 0.22f : -0.05f; break;

    case EBossTacticalAction::CircleLeft:
    case EBossTacticalAction::CircleRight: modifiedScore += _context.bPlayerAiming ? 0.18f : 0.02f; break;

    case EBossTacticalAction::Retreat:
        //「GetHealthRatio() < 0.35f」が成立するとき、後続コードへ不正な参照や利用できない状態を渡さないようにします。
        if (GetHealthRatio() < 0.35f)
        {
            modifiedScore += 0.15f;
        }
        break;

    default: break;
    }

    //modifiedScoreは、ゲーム判定に使用する数値を計算し、後続の比較または更新へ渡すために使います。
    return modifiedScore;
}
