#pragma once

#include "CoreMinimal.h"
#include "BossAITypes.generated.h"

//ボスの戦い方を切り替える種類
UENUM(BlueprintType)
enum class EBossAIArchetype : uint8
{
    //攻撃と防御を均等に使う標準型
    Balanced,
    //中間ボス向けの戦闘型
    MidBoss,
    //ラストボス向けの戦闘型
    FinalBoss
};

//戦術行動の列挙型
UENUM(BlueprintType)
enum class EBossTacticalAction : uint8
{
    //距離を保ちながらプレイヤーの行動を見る
    Observe,
    //攻撃できる距離まで近づく
    Approach,
    //プレイヤーの左側へ回り込む
    CircleLeft,
    //プレイヤーの右側へ回り込む
    CircleRight,
    //安全な距離まで後退する
    Retreat,
    //隙の少ない連続攻撃を行う
    LightCombo,
    //地面を叩く範囲攻撃を行う
    PowerSlam,
    //プレイヤーへ突進する
    ChargeRush,
    //攻撃を避けるため後方へ跳ぶ
    BackStep
};

//ボスの意思決定コンテキストを表す構造体
USTRUCT(BlueprintType)
struct FBossDecisionContext
{
    //エンジンが使う定型コード
    GENERATED_BODY()

    //プレイヤーとの距離
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float DistanceToPlayer = 0.f;

    //ボスの体力比率
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float BossHealthRatio = 1.f;

    //プレイヤーの体力比率
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float PlayerHealthRatio = 1.f;

    //プレイヤーの速度
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float PlayerSpeed = 0.f;

    //プレイヤーの横方向の速度
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float PlayerLateralSpeed = 0.f;

    //遠距離攻撃を受けている度合い
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float RangedPressure = 0.f;

    //プレイヤーの機動性の圧力
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float MobilityPressure = 0.f;

    //ボスの脆弱性の圧力
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float VulnerabilityPressure = 0.f;

    //最近のボスへのダメージ圧力
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float RecentBossDamagePressure = 0.f;

    //最近のプレイヤーへのヒット圧力
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float RecentSuccessfulHitPressure = 0.f;

    //プレイヤーを直接視認できるかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool bHasLineOfSight = false;

    //プレイヤーが照準を合わせているかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool bPlayerAiming = false;

    //プレイヤーがリロード中かどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool bPlayerReloading = false;

    //プレイヤーが回復中かどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool bPlayerHealing = false;

    //プレイヤーが武器を切り替えているかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool bPlayerSwitchingWeapon = false;

    //プレイヤーが遠距離武器を使用しているかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool bPlayerUsingRangedWeapon = true;

    //プレイヤーがボスに向かって移動しているかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool bPlayerMovingTowardBoss = false;

    //プレイヤーがボスから離れて移動しているかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool bPlayerMovingAwayFromBoss = false;

    //ボスがフェーズ2に入っているかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool bBossPhaseTwo = false;

    //現在の動きから予測したプレイヤーの位置
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    FVector PredictedPlayerLocation = FVector::ZeroVector;
};
