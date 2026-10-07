
#include "DayNightCycleManager.h"
#include "EngineUtils.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/SkyLight.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PostProcessVolume.h"
#include "Components/SkyLightComponent.h"

//昼夜サイクルの初期値を設定します。
ADayNightCycleManager::ADayNightCycleManager()
    : m_pSunlightActor(nullptr), m_pSkyLightActor(nullptr), m_dayLength(180.f), m_dayRatio(0.7f), m_skyRecaptureInterval(2.f), m_daySunIntensity(6.f),
      m_nightSunIntensity(7.5f), m_daySkyIntensity(1.0f), m_nightSkyIntensity(1.6f), m_gameplayExposureBias(-0.45f), m_timeOfDay(0.f),
      m_skyRecaptureTimer(0.f)
{
    //時刻と太陽位置を継続更新するためTickを有効にします。
    PrimaryActorTick.bCanEverTick = true;
}

//ゲーム開始時にライト参照を解決します。
void ADayNightCycleManager::BeginPlay()
{
    //ゲーム開始時に必要な参照を取得し、初期状態を整えます。
    Super::BeginPlay();

    //固定時刻のステージではエディタの照明をそのまま使い、不要な毎フレーム更新も止めます。
    if (m_useLevelLighting) { SetActorTickEnabled(false); return; }

    //配置済みのPostProcessVolumeを本編用の露出へ統一します。
    ApplyGameplayPostProcess();

    //Blueprintで太陽が指定されていない場合は、レベル内のDirectionalLightを使用します。
    if (!m_pSunlightActor)
    {
        //最初に見つかったDirectionalLightを昼夜演出の太陽として登録します。
        for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
        {
            m_pSunlightActor = *It;
            break;
        }
    }

    //BlueprintでSkyLightが指定されていない場合は、レベル内から自動取得します。
    if (!m_pSkyLightActor)
    {
        //最初に見つかったSkyLightを間接光の調整対象として登録します。
        for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
        {
            m_pSkyLightActor = *It;
            break;
        }
    }
    if (m_pSkyLightActor && m_pSkyLightActor->GetLightComponent())
    {
        m_pSkyLightActor->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        m_pSkyLightActor->GetLightComponent()->SetRealTimeCaptureEnabled(true);
    }

    //太陽の回転と照度を実行中に変更できるよう、ライトをMovableにします。
    if (m_pSunlightActor && m_pSunlightActor->GetLightComponent())
    {
        m_pSunlightActor->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    }
}

//レベル内のPostProcessVolumeを同じ露出へ揃え、場所による白飛びの差を防ぎます。
void ADayNightCycleManager::ApplyGameplayPostProcess()
{
    //重なっているVolumeも含めて露出を統一し、優先度による明るさの急変を防ぎます。
    for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
    {
        APostProcessVolume* PostProcessVolume = *It;
        //削除待ちのVolumeには描画設定を適用しません。
        if (!IsValid(PostProcessVolume))
        {
            continue;
        }

        //空と霧の白飛びを抑える露出をVolume側の最終設定として使用します。
        PostProcessVolume->Settings.bOverride_AutoExposureBias = true;
        PostProcessVolume->Settings.AutoExposureBias = m_gameplayExposureBias;
    }
}

//毎フレームの更新を行います。
void ADayNightCycleManager::Tick(float _deltaTime)
{
    //フレームごとの経過時間を使って、移動や表示の変化を更新します。
    Super::Tick(_deltaTime);

    //無効な周期では除算を行わず、現在の照明を維持します。
    if (m_useLevelLighting || m_dayLength <= 0.f) { return; }

    m_timeOfDay += _deltaTime / m_dayLength;
    //一日の終端を越えた時刻を先頭へ戻します。
    if (m_timeOfDay > 1.f)
    {
        m_timeOfDay -= 1.f;
    }

    UpdateSunLight();

    //空の再キャプチャは毎フレーム行わず、指定間隔まで時間を蓄積します。
    m_skyRecaptureTimer += _deltaTime;
    //空の色が十分変化したタイミングだけSkyLightを更新します。
    if (m_skyRecaptureTimer >= m_skyRecaptureInterval)
    {
        //SkyLightがレベルに存在するときだけ再キャプチャを実行します。
        if (m_pSkyLightActor && m_pSkyLightActor->GetLightComponent())
        {
            //SkyLightComponentへ変換できた場合に空の照明情報を取り直します。
            if (USkyLightComponent* SkyComp = Cast<USkyLightComponent>(m_pSkyLightActor->GetLightComponent()))
            {
                SkyComp->RecaptureSky();
            }
        }
        m_skyRecaptureTimer = 0.f;
    }
}

//現在時刻から太陽位置と明るさを決め、夜間の黒潰れを抑えます。
void ADayNightCycleManager::UpdateSunLight()
{
    //太陽が存在しないレベルでは照明更新を行いません。
    if (!m_pSunlightActor) { return; }

    //現在時刻から求めた太陽の上下角です。
    float SunPitch;
    //昼と夜の照明設定を切り替える係数です。
    float DayFactor;

    //昼の時間帯は太陽を東から西へ移動させます。
    if (m_timeOfDay < m_dayRatio)
    {
        //昼の経過率を太陽の回転角へ変換します。
        const float NormalizedDay = m_timeOfDay / m_dayRatio;
        SunPitch = FMath::Lerp(5.f, -120.f, NormalizedDay);
        DayFactor = 1.f;
    }
    else
    {
        //夜の経過率を使って太陽を開始位置へ戻します。
        const float NightProgress = (m_timeOfDay - m_dayRatio) / (1.f - m_dayRatio);
        SunPitch = FMath::Lerp(-120.f, 5.f, NightProgress);
        DayFactor = 0.f;
    }

    m_pSunlightActor->SetActorRotation(FRotator(SunPitch, 0.f, 0.f));

    //太陽光には昼夜別の照度と色温度を適用します。
    if (auto* LightComp = m_pSunlightActor->GetLightComponent())
    {
        //夜間の照度を残し、建物の陰でも敵のシルエットが読めるようにします。
        const float Intensity = DayFactor > 0.7f ? m_daySunIntensity : m_nightSunIntensity;
        LightComp->SetIntensity(Intensity);

        //夜は薄い青を加え、ホラーらしい暗さと輪郭の読みやすさを両立します。
        const FLinearColor NightColor(0.62f, 0.72f, 0.92f);
        //昼は素材本来の色が崩れない白色光を使います。
        const FLinearColor DayColor = FLinearColor::White;
        LightComp->SetLightColor(DayFactor > 0.7f ? DayColor : NightColor);
    }

    //SkyLightで暗部を持ち上げ、黒一色になる場所を減らします。
    if (m_pSkyLightActor && m_pSkyLightActor->GetLightComponent())
    {
        //取得したライトがSkyLightComponentの場合に間接光を調整します。
        if (USkyLightComponent* SkyComp = Cast<USkyLightComponent>(m_pSkyLightActor->GetLightComponent()))
        {
            //夜間も最低限の間接光を残し、敵・武器・進行方向を識別可能にします。
            const float SkyIntensity = DayFactor > 0.7f ? m_daySkyIntensity : m_nightSkyIntensity;
            SkyComp->SetIntensity(SkyIntensity);
        }
    }
}
