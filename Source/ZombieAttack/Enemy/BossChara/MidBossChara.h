#pragma once

#include "CoreMinimal.h"
#include "BossChara.h"
#include "MidBossChara.generated.h"

//Midボスキャラクターの動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API AMidBossChara : public ABossChara
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //コンストラクタ
    AMidBossChara();

    //毎フレーム呼ばれる関数
    virtual EBossAIArchetype GetBossAIArchetype() const override;

    //BossUtilityAIのスコアをボス種別ごとに微調整する関数
    virtual float ModifyUtilityScore(EBossTacticalAction _action, const FBossDecisionContext& _context,
                                     //overrideをゲーム処理から参照できるように管理します。
                                     float _baseScore) const override;
};
