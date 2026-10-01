#include "MidBossChara.h"

//コンストラクタ
AMidBossChara::AMidBossChara()
{
    SetLocomotionAssets(TEXT("/Game/Assets/Enemy/Animation/MidBoss/AnimationSequence"), TEXT("Drunk_Idle_Variation"),
                        TEXT("Walking"), TEXT("Zombie_Run"), TEXT("Zombie_Scream"));
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
    case EBossTacticalAction::LightCombo: modifiedScore += 0.15f + (_context.m_bPlayerReloading ? 0.28f : 0.0f); break;

    case EBossTacticalAction::PowerSlam: modifiedScore += 0.05f; break;

    case EBossTacticalAction::ChargeRush: modifiedScore += (_context.m_bPlayerHealing || _context.m_bPlayerReloading) ? 0.22f : -0.05f; break;

    case EBossTacticalAction::CircleLeft:
    case EBossTacticalAction::CircleRight: modifiedScore += _context.m_bPlayerAiming ? 0.18f : 0.02f; break;

    case EBossTacticalAction::Retreat:
        if (GetHealthRatio() < 0.35f)
        {
            modifiedScore += 0.15f;
        }
        break;

    default: break;
    }
    return modifiedScore;
}
