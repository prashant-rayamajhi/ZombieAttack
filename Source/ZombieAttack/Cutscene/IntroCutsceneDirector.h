#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IntroCutsceneDirector.generated.h"

//前方宣言
class ACameraActor;
//ASpawnEnemiesは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class ASpawnEnemies;
//UPointLightComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UPointLightComponent;
//UUserWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UUserWidget;

//IntroHUDRevealEntryに必要な値をまとめた構造体
struct FIntroHUDRevealEntry
{
    //Widgetをゲーム処理から参照できるように管理します。
    TWeakObjectPtr<UUserWidget> Widget;
    //Delayを秒単位で指定します。
    float Delay = 0.0f;
    //Elapsedをゲーム処理から参照できるように管理します。
    float Elapsed = 0.0f;
    //StartOffsetは、FVector2D(-140.0f, 0.0f)から求めた空間情報を位置または向きの計算に使います。
    FVector2D StartOffset = FVector2D(-140.0f, 0.0f);
    //RestingOffsetをゲーム処理から参照できるように管理します。
    FVector2D RestingOffset = FVector2D::ZeroVector;
};

//AIntroカットシーンDirectorの動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API AIntroCutsceneDirector : public AActor
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //コンストラクタ
    AIntroCutsceneDirector();

  protected:
    //ゲーム開始時に呼ばれる関数
    virtual void BeginPlay() override;

    //毎フレーム呼ばれる関数
    virtual void Tick(float _deltaTime) override;

  private:
    //レベル内のSpawnerを自動検出し、BP設定漏れでもミッションを開始できるようにします。
    void DiscoverSpawnActors();
    //ミッションSubjectShotsを作成します。
    void BuildMissionSubjectShots();

    //カットシーンを開始する関数
    void StartCutscene();

    //カットシーンを終了する関数
    void FinishCutscene();

    //カットシーン中に生成済み・遅延生成されたクロスヘアをまとめて制御します。
    void SetCutsceneCrosshairSuppressed(bool _bSuppressed);

    //カットシーン中の戦闘HUDをまとめて非表示にする
    void SetGameplayHUDHidden();

    //ミッション表示に続けて、戦闘HUDを順番にスライド表示する
    void BeginGameplayHUDReveal();
    //UpdateGameplayHUDRevealは、引数の内容をゲーム中の状態または表示へ反映します。
    void UpdateGameplayHUDReveal(float _deltaTime);

    //プレイヤーの操作を有効/無効にする関数
    void SetPlayerControlEnabled(bool _bEnabled);

    //カットシーン中に使用するカメラを作成する関数
    void CreateRuntimeCameraIfNeeded();

    //カメラを指定したターゲットにスナップする関数
    void SnapRuntimeCameraToTarget(int32 _targetIndex);

    //カメラを次のターゲットに移動する関数
    void StartMoveToNextTarget();

    //カメラを次のターゲットに移動する関数
    void UpdateCinematicCamera(float _deltaTime);

    //カメラを次のターゲットに移動する関数
    bool IsValidViewTargetIndex(int32 _targetIndex) const;

    //カメラのターゲットのTransformを取得する関数
    FTransform GetViewTargetTransform(int32 _targetIndex) const;

    //カメラのターゲットのLookAt回転を計算する関数
    FRotator CalculateLookAtRotation(const FVector& _cameraLocation, int32 _targetIndex) const;

    FVector ResolveObstructionSafeLocation(const FVector& _desiredLocation,
                                           //constをゲーム処理から参照できるように管理します。
                                           int32 _targetIndex) const;

    //移動開始点と終了点の両方から衝突しない、曲線移動用の中継点を探します。
    FVector BuildTravelControlLocation(const FVector& _startLocation,
                                       //constをゲーム処理から参照できるように管理します。
                                       const FVector& _endLocation) const;

    //移動中の1フレーム分をSweepし、壁や木の内部へカメラが入ることを防ぎます。
    FVector ResolveTravelCollision(const FVector& _currentLocation,
                                   //constをゲーム処理から参照できるように管理します。
                                   const FVector& _desiredLocation) const;


    //カメラのFOVを計算する関数
    float EaseInOut(float _alpha) const;

  private:
    //カメラの通過地点
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Camera")
    TArray<TObjectPtr<AActor>> m_viewTargets;

    //各カットで見たい対象
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Camera")
    TArray<TObjectPtr<AActor>> m_lookAtTargets;

    //1カットの移動時間
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Timing", meta = (ClampMin = "0.1"))
    float m_moveDurationPerTarget;

    //各カット到着後に少し止める時間
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Timing", meta = (ClampMin = "0.0"))
    float m_holdDurationAtTarget;

    //最後にプレイヤーへ戻る時のブレンド時間
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Timing", meta = (ClampMin = "0.0"))
    float m_returnBlendTime;

    //シネマチック感を出すため、プレイヤーカメラへ戻る前に少し待つ時間
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Timing", meta = (ClampMin = "0.0"))
    float m_endHoldDuration;

    //遮蔽物を隠す暗転は短くし、黒画面より各紹介ショットを長く見せます。
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Timing", meta = (ClampMin = "0.01"))
    float m_fadeOutDuration;

    //hiddenTravel時間を秒単位で指定します。
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Timing", meta = (ClampMin = "0.01"))
    //hiddenTravel時間を秒単位で指定します。
    float m_hiddenTravelDuration;

    //fadeIn時間を秒単位で指定します。
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Timing", meta = (ClampMin = "0.01"))
    //fadeIn時間を秒単位で指定します。
    float m_fadeInDuration;

    //minimumShotHold時間を秒単位で指定します。
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Timing", meta = (ClampMin = "0.0"))
    //minimumShotHold時間を秒単位で指定します。
    float m_minimumShotHoldDuration;

    //カメラのFOV
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Camera", meta = (ClampMin = "5.0", ClampMax = "170.0"))
    //cameraFOVをゲーム処理から参照できるように管理します。
    float m_cameraFOV;

    //カメラ移動にEase In / Ease Outを使うかどうかです。
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Camera")
    bool m_bUseSmoothEase;

    //AvoidカメラObstructionsかを示します。
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Camera")
    //AvoidカメラObstructionsかを示します。
    bool m_bAvoidCameraObstructions;

    //BPの注視対象が古い、または未設定でも敵出現地点を必ず映します。
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Camera")
    bool m_bAutoFrameMissionSubjects;

    //obstructionProbe範囲の調整値です。
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Camera", meta = (ClampMin = "10.0"))
    //obstructionProbe範囲の調整値です。
    float m_obstructionProbeRadius;

    //obstructionLiftをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Camera", meta = (ClampMin = "0.0"))
    //obstructionLiftをゲーム処理から参照できるように管理します。
    float m_obstructionLift;

    //カットシーン終了後に起動するSpawnerです。
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene|Spawn")
    TArray<TObjectPtr<ASpawnEnemies>> m_spawnActorsToActivate;

    //カットシーン開始時に自動で再生するかどうか
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene")
    bool m_bPlayOnBeginPlay;

    //カットシーン中にプレイヤーの操作を無効化するかどうか
    UPROPERTY(EditAnywhere, Category = "Intro Cutscene")
    bool m_bDisablePlayerControlDuringCutscene;

    //デバッグ用 Editor上でカメラ順番確認したい場合だけONにします。

  private:
    //カットシーン中に使用するカメラ
    UPROPERTY()
    TObjectPtr<ACameraActor> m_runtimeCamera;

    //cinematicFillLightをゲーム処理から参照できるように管理します。
    UPROPERTY()
    //cinematicFillLightをゲーム処理から参照できるように管理します。
    TObjectPtr<UPointLightComponent> m_cinematicFillLight;

    //プレイヤーコントローラーとプレイヤーポーンの弱参照
    TWeakObjectPtr<APlayerController> m_playerController;
    //layerPawnの操作に使用する参照です。
    TWeakObjectPtr<APawn> m_playerPawn;

    //現在のターゲットインデックスと次のターゲットインデックス
    int32 m_currentTargetIndex;
    //nextTargetIndexを管理します。
    int32 m_nextTargetIndex;

    //カメラ移動の経過時間と停止の経過時間
    float m_moveElapsedTime;
    //holdElapsedTimeを秒単位で指定します。
    float m_holdElapsedTime;
    //endHoldElapsedTimeを秒単位で指定します。
    float m_endHoldElapsedTime;

    //カットシーンの状態を追跡するフラグ
    bool m_bCutscenePlaying;
    //MovingToNextTargetかを示します。
    bool m_bMovingToNextTarget;
    //WaitingAtTargetかを示します。
    bool m_bWaitingAtTarget;
    //Endingカットシーンかを示します。
    bool m_bEndingCutscene;

    //カメラの移動開始時と終了時のTransformを保持する変数
    FTransform m_moveStartTransform;
    //moveEndTransformをゲーム処理から参照できるように管理します。
    FTransform m_moveEndTransform;
    //moveControl位置をゲーム処理から参照できるように管理します。
    FVector m_moveControlLocation;
    //hudRevealEntriesをゲーム処理から参照できるように管理します。
    TArray<FIntroHUDRevealEntry> m_hudRevealEntries;
    //HUDRevealPlayingかを示します。
    bool m_bHUDRevealPlaying = false;
};
