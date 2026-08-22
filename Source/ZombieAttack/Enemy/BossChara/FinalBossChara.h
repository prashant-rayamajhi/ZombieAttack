#pragma once

#include "CoreMinimal.h"
#include "BossChara.h"
#include "FinalBossChara.generated.h"

//Finalボスキャラクターの動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API AFinalBossChara : public ABossChara
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //コンストラクタ
    AFinalBossChara();

    //BossAIControllerが次の行動を選んでよいか確認するための関数
    virtual EBossAIArchetype GetBossAIArchetype() const override;

    //UtilityAIのスコアをボス種別ごとに微調整する関数
    virtual float ModifyUtilityScore(EBossTacticalAction _action, const FBossDecisionContext& _context,
                                     //overrideをゲーム処理から参照できるように管理します。
                                     float _baseScore) const override;
};
