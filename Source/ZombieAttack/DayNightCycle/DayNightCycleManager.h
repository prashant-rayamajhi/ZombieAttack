#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DayNightCycleManager.generated.h"

//前方宣言
class ADirectionalLight;
//ASkyLightは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class ASkyLight;

//暗い雰囲気を保ちながら、時刻に応じて戦闘に必要な視認性を整えるクラス
UCLASS()
class ZOMBIEATTACK_API ADayNightCycleManager : public AActor
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //昼夜サイクルを動かすための初期値を設定します。
    ADayNightCycleManager();

  protected:
    //ゲーム開始時の初期設定を行います。
    virtual void BeginPlay() override;

  public:
    //毎フレームの更新を行います。
    virtual void Tick(float _deltaTime) override;

  protected:
    //太陽光源アクター
    UPROPERTY(EditAnywhere, Category = "DayNight")
    ADirectionalLight* m_pSunlightActor;

    //スカイライトアクター
    UPROPERTY(EditAnywhere, Category = "DayNight")
    ASkyLight* m_pSkyLightActor;

    //1日の長さ（秒）
    UPROPERTY(EditAnywhere, Category = "DayNight")
    float m_dayLength;

    //1日のうち太陽が出ている時間の割合です。
    UPROPERTY(EditAnywhere, Category = "DayNight")
    float m_dayRatio;

    //SkyLightを再キャプチャする間隔です。短くしすぎると描画負荷が増えます。
    UPROPERTY(EditAnywhere, Category = "DayNight")
    float m_skyRecaptureInterval;

    //昼間に使う太陽光の照度です。
    UPROPERTY(EditAnywhere, Category = "DayNight|Visibility", meta = (ClampMin = "0.0"))
    float m_daySunIntensity;

    //夜間でも敵と足元の輪郭を失わないための太陽光照度です。
    UPROPERTY(EditAnywhere, Category = "DayNight|Visibility", meta = (ClampMin = "0.0"))
    float m_nightSunIntensity;

    //昼間の間接光を決めるSkyLightの強さです。
    UPROPERTY(EditAnywhere, Category = "DayNight|Visibility", meta = (ClampMin = "0.0"))
    float m_daySkyIntensity;

    //暗部を完全な黒に潰さないための夜間SkyLightの強さです。
    UPROPERTY(EditAnywhere, Category = "DayNight|Visibility", meta = (ClampMin = "0.0"))
    float m_nightSkyIntensity;

    //GameLevel1内のPostProcessVolumeへ適用する露出補正です。
    UPROPERTY(EditAnywhere, Category = "DayNight|Visibility", meta = (ClampMin = "-3.0", ClampMax = "3.0"))
    float m_gameplayExposureBias;

  private:
    //現在の時刻（0.0〜1.0）
    float m_timeOfDay;

    //次のSkyLight再キャプチャまでの経過時間です。
    float m_skyRecaptureTimer;

    //現在時刻から太陽の向き、色、直接光、間接光を更新します。
    void UpdateSunLight();

    //レベル内のPostProcessVolumeを同じ露出へ揃え、場所による白飛びの差を防ぎます。
    void ApplyGameplayPostProcess();
};
