#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnEnemies.generated.h"

//前方宣言
class UEnemyCount;
//AEnemyCharaは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class AEnemyChara;
//ABaseCharacterは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class ABaseCharacter;
//AGoalActorは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class AGoalActor;

//Waveの敵データを保持する構造体です。
USTRUCT(BlueprintType)
struct FWaveEnemyData
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //m_enemiesToSpawnをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
    //m_enemiesToSpawnをゲーム処理から参照できるように管理します。
    int32 m_enemiesToSpawn = 0;

    //m_spawnIntervalを秒単位で指定します。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
    //m_spawnIntervalを秒単位で指定します。
    float m_spawnInterval = 2.0f;

    //wave敵クラスesをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
    //wave敵クラスesをゲーム処理から参照できるように管理します。
    TArray<TSubclassOf<AEnemyChara>> m_waveEnemyClasses;

    //bossクラスをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Boss")
    //bossクラスをゲーム処理から参照できるように管理します。
    TSubclassOf<AEnemyChara> m_bossClass;
};

//生成Enemiesの動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API ASpawnEnemies : public AActor
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    ASpawnEnemies();

  public:
    //Waveの敵を順番に生成する処理を開始する関数
    UFUNCTION(BlueprintCallable, Category = "Spawn")
    void ActivateSpawn();

    //Waveの敵を順番に生成する処理を停止する関数
    UFUNCTION(BlueprintCallable, Category = "Spawn|Intro")
    void PrepareAllEnemiesForIntro();

    //PrepareAllEnemiesForIntro()で生成された敵を解放する関数
    UFUNCTION(BlueprintCallable, Category = "Spawn|Intro")
    void ReleasePreparedEnemies();
    static void NotifyGoalActivated(UWorld* _pWorld);

  protected:
    //AActorのBeginPlay()とTick()をオーバーライドします。
    virtual void BeginPlay() override;
    //毎フレームの更新を行います。
    virtual void Tick(float _deltaTime) override;

  protected:
    //Wave側にEnemyClassが設定されていない場合に使うフォールバック
    UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Default Enemy Classes (Fallback)"))
    //enemyクラスesをゲーム処理から参照できるように管理します。
    TArray<TSubclassOf<AEnemyChara>> m_enemyClasses;

    //Wave側にBossClassが設定されていない場合に使うフォールバック
    UPROPERTY(EditAnywhere, Category = "Spawn|Rectangle")
    float m_spawnHalfExtentX;

    //spawnHalfExtentYをゲーム処理から参照できるように管理します。
    UPROPERTY(EditAnywhere, Category = "Spawn|Rectangle")
    //spawnHalfExtentYをゲーム処理から参照できるように管理します。
    float m_spawnHalfExtentY;

    //生成範囲を円形にする場合の半径
    UPROPERTY(EditAnywhere, Category = "Spawn")
    float m_spawnRadius;

    //全Spawnerで共有する敵数UIのクラス
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UEnemyCount> m_enemyCount;

    //全Spawnerで共有する敵数UIのインスタンス
    UPROPERTY()
    TObjectPtr<UEnemyCount> m_pEnemyCountWidget;

    //Waveの敵データを保持する配列
    UPROPERTY(EditAnywhere, Category = "Wave System")
    TArray<FWaveEnemyData> m_waves;

    //現在のWaveのインデックス
    UPROPERTY(VisibleAnywhere, Category = "Wave System")
    int32 m_currentWaveIndex;

    //Waveの敵を全て倒したかどうかのフラグ
    UPROPERTY(VisibleAnywhere, Category = "Wave System")
    int32 m_remainingEnemies;

    //Waveの最大インデックス
    UPROPERTY(EditAnywhere, Category = "Wave System")
    int32 m_maxWaveIndex;

    //Waveの敵を全て同時に生成するかどうかのフラグ
    UPROPERTY(EditAnywhere, Category = "Wave System|Mission", meta = (DisplayName = "Spawn All Waves At Once"))
    //SpawnAllWavesAtOnceかを示します。
    bool m_bSpawnAllWavesAtOnce;

    //Waveの敵をイントロ中に凍結させるかどうかのフラグ
    UPROPERTY(EditAnywhere, Category = "Wave System|Mission", meta = (DisplayName = "Freeze Enemies During Intro"))
    //FreezeEnemiesDuringIntroかを示します。
    bool m_bFreezeEnemiesDuringIntro;

    //Waveの敵を順番に生成する処理を自動的に開始するかどうかのフラグ
    UPROPERTY(EditAnywhere, Category = "Spawn|Sequential", meta = (DisplayName = "Auto Start (No Cutscene Only)"))
    //AutoStartかを示します。
    bool m_bAutoStart;

    //Waveの敵を順番に生成する処理を開始する次のSpawnerの配列
    UPROPERTY(EditAnywhere, Category = "Spawn|Sequential", meta = (DisplayName = "Next Spawn Actors (Sequential Mode Only)"))
    //nextSpawnActorsをゲーム処理から参照できるように管理します。
    TArray<TObjectPtr<ASpawnEnemies>> m_nextSpawnActors;

    //Waveの敵を全て倒した後に出現するゴールActor
    UPROPERTY(EditAnywhere, Category = "Spawn|Goal", meta = (DisplayName = "Goal Actor"))
    TObjectPtr<AGoalActor> m_pGoalActor;

  private:
    //現在生成されている敵の配列
    UPROPERTY()
    TArray<TObjectPtr<AEnemyChara>> m_spawnedEnemies;

    //敵が死亡したときに呼ばれる関数
    UFUNCTION()
    void OnEnemyDeath(AActor* _destroyedActor);

    //Waveを開始する処理を行う関数
    void StartNextWave();

    //Waveを終了する処理を行う関数
    void EndWave();

    //Waveの敵を順番に生成する処理を行う関数
    void SpawnAllWaves(bool _bFreezeAfterSpawn);

    //Waveの敵を順番に生成する処理を行う関数
    void SpawnWaveEnemy(int32 _count, const TArray<TSubclassOf<AEnemyChara>>& _enemyClasses);

    //Waveのボスを生成する処理を行う関数
    void SpawnBoss(TSubclassOf<AEnemyChara> _bossClass);

    //Waveの敵を生成する処理を行う関数
    FVector GetRandomSpawnPointInRect() const;

    //エネミーUIを確保する関数
    void EnsureEnemyCountWidget();

    //Waveの敵数UIを更新する関数
    void UpdateEnemyCountWidget();

    //Waveの敵クラスの有効数をカウントする関数
    int32 CountValidEnemyClasses(const TArray<TSubclassOf<AEnemyChara>>& _enemyClasses) const;

    //Waveの敵を全て倒したかどうかをチェックする関数
    void SetEnemyGameplayEnabled(AEnemyChara* _enemy, bool _bEnabled);
    //SetAllSpawnedEnemyGameplayEnabledは、引数の内容をゲーム中の状態または表示へ反映します。
    void SetAllSpawnedEnemyGameplayEnabled(bool _bEnabled);
    //GetMissionRemainingEnemyCountは、呼び出し元が必要とする対象または計算結果を返します。
    int32 GetMissionRemainingEnemyCount() const;
    //TryActivateGoalAfterAllEnemiesDefeatedは、必要条件を検査して実行可能な場合だけ名前が示す動作を開始します。
    void TryActivateGoalAfterAllEnemiesDefeated();

  protected:
    //ゲームクリア時の処理を行う関数
    void HandleGameClear();

    //ゲームオーバー時の処理を行う関数
    UFUNCTION()
    void HandleGameOver(ABaseCharacter* _deadCharacter);

  private:
    //Waveの敵を順番に生成する処理を行うタイマー
    FTimerHandle m_waveTimerHandle;
    //gameOverTimerHandleを秒単位で指定します。
    FTimerHandle m_gameOverTimerHandle;

    //ゲームオーバー時のフラグ
    bool m_bIsGameOver;
    //IsActivatedかを示します。
    bool m_bIsActivated;
    //PreparedForIntroかを示します。
    bool m_bPreparedForIntro;
    //EnemiesReleasedかを示します。
    bool m_bEnemiesReleased;
};
