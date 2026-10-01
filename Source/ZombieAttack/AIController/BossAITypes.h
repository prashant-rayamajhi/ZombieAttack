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
    float m_distanceToPlayer = 0.f;

    //ボスの体力比率
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float m_bossHealthRatio = 1.f;

    //プレイヤーの体力比率
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float m_playerHealthRatio = 1.f;

    //プレイヤーの速度
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float m_playerSpeed = 0.f;

    //プレイヤーの横方向の速度
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float m_playerLateralSpeed = 0.f;

    //遠距離攻撃を受けている度合い
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float m_rangedPressure = 0.f;

    //プレイヤーの機動性の圧力
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float m_mobilityPressure = 0.f;

    //ボスの脆弱性の圧力
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float m_vulnerabilityPressure = 0.f;

    //最近のボスへのダメージ圧力
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float m_recentBossDamagePressure = 0.f;

    //最近のプレイヤーへのヒット圧力
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    float m_recentSuccessfulHitPressure = 0.f;

    //プレイヤーを直接視認できるかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool m_bHasLineOfSight = false;

    //プレイヤーが照準を合わせているかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool m_bPlayerAiming = false;

    //プレイヤーがリロード中かどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool m_bPlayerReloading = false;

    //プレイヤーが回復中かどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool m_bPlayerHealing = false;

    //プレイヤーが武器を切り替えているかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool m_bPlayerSwitchingWeapon = false;

    //プレイヤーが遠距離武器を使用しているかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool m_bPlayerUsingRangedWeapon = true;

    //プレイヤーがボスに向かって移動しているかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool m_bPlayerMovingTowardBoss = false;

    //プレイヤーがボスから離れて移動しているかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool m_bPlayerMovingAwayFromBoss = false;

    //ボスがフェーズ2に入っているかどうか
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    bool m_bBossPhaseTwo = false;

    //現在の動きから予測したプレイヤーの位置
    UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
    FVector m_predictedPlayerLocation = FVector::ZeroVector;
};
