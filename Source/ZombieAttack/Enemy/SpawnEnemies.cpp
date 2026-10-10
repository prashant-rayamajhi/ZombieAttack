#include "SpawnEnemies.h"

#include "AIController.h"
#include "BossChara/FinalBossChara.h"
#include "EnemyChara.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "ZombieAttack/Character/BaseCharacter.h"
#include "ZombieAttack/Player/PlayerChara.h"
#include "ZombieAttack/Goal/GoalActor.h"
#include "ZombieAttack/UI/EnemyUI/EnemyCount.h"
#include "ZombieAttack/UI/Configuration/ZombieAttackUISettings.h"

//グローバル変数として、敵の残り数を表示するウィジェットを保持する
namespace
{
TWeakObjectPtr<UEnemyCount> g_enemyCountWidget;
}

//コンストラクタ
ASpawnEnemies::ASpawnEnemies()
    : m_spawnHalfExtentX(500.0f), m_spawnHalfExtentY(500.0f), m_spawnRadius(1000.0f), m_pEnemyCountWidget(nullptr), m_currentWaveIndex(-1),
      m_remainingEnemies(0), m_maxWaveIndex(3), m_bSpawnAllWavesAtOnce(true), m_bFreezeEnemiesDuringIntro(true), m_bAutoStart(false),
      m_pGoalActor(nullptr), m_bIsGameOver(false), m_bIsActivated(false), m_bPreparedForIntro(false), m_bEnemiesReleased(false)
{
    //Tickを有効にする
    PrimaryActorTick.bCanEverTick = true;
}

//ゲーム開始時に呼ばれる関数
void ASpawnEnemies::BeginPlay()
{
    //親クラスのBeginPlayを呼び出す
    Super::BeginPlay();

    //プレイヤーキャラクターを取得
    APawn* playerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    //プレイヤーキャラクターを保持します。
    ABaseCharacter* playerCharacter = Cast<ABaseCharacter>(playerPawn);
    if (playerCharacter)
    {
        playerCharacter->m_onCharacterDied.AddDynamic(this, &ASpawnEnemies::HandleGameOver);
    }

    //ゲーム開始時に自動的にスポーンを開始する場合は、ActivateSpawn()を呼び出す
    if (m_bAutoStart)
    {
        ActivateSpawn();
    }
}

//毎フレーム呼ばれる関数
void ASpawnEnemies::Tick(float _deltaTime)
{
    //親クラスのTickを呼び出す
    Super::Tick(_deltaTime);
}

//ゲームオーバー時に呼ばれる関数
void ASpawnEnemies::ActivateSpawn()
{
    //ゲームオーバー時はスポーンを開始しない
    if (m_bIsGameOver) { return; }

    //すでにIntro用に生成済みなら、ここでは生成せずAIだけ解放する
    if (m_bPreparedForIntro)
    {
        ReleasePreparedEnemies();
        return;
    }

    //すでにスポーン済みなら何もしない
    if (m_bIsActivated) { return; }

    //スポーンを開始するフラグを立てる
    m_bIsActivated = true;
    EnsureEnemyCountWidget();

    //全Waveを一度に生成するか、順番に生成するかで処理を分ける
    if (m_bSpawnAllWavesAtOnce)
    {
        SpawnAllWaves(false);
    }
    else
    {
        StartNextWave();
    }
}

//ゲームオーバー時に呼ばれる関数
void ASpawnEnemies::PrepareAllEnemiesForIntro()
{
    //ゲームオーバー時はスポーンを開始しない
    if (m_bIsGameOver) { return; }

    //すでにIntro用に生成済み、またはスポーン済みなら何もしない
    if (m_bPreparedForIntro || m_bIsActivated) { return; }

    //スポーンを開始するフラグを立てる
    m_bIsActivated = true;
    m_bPreparedForIntro = true;
    m_bEnemiesReleased = false;

    //ウィジェットを確実に生成する
    EnsureEnemyCountWidget();
    SpawnAllWaves(m_bFreezeEnemiesDuringIntro);
}

//カットシーン終了時に呼ばれる関数
void ASpawnEnemies::ReleasePreparedEnemies()
{
    //ゲームオーバー時、またはすでに敵を解放済みなら何もしない
    if (m_bIsGameOver || m_bEnemiesReleased) { return; }

    //すでにIntro用に生成済みで、敵を解放するフラグを立てる
    m_bEnemiesReleased = true;
    SetAllSpawnedEnemyGameplayEnabled(true);
    UpdateEnemyCountWidget();
    if (g_enemyCountWidget.IsValid() && g_enemyCountWidget->GetWorld() == GetWorld())
    {
        g_enemyCountWidget->ShowMissionObjective(GetMissionRemainingEnemyCount());
    }

}

//ウィジェットを確実に生成する関数
void ASpawnEnemies::EnsureEnemyCountWidget()
{
    if (g_enemyCountWidget.IsValid() && g_enemyCountWidget->GetWorld() == GetWorld())
    {
        m_pEnemyCountWidget = g_enemyCountWidget.Get();
        return;
    }

    //ウィジェットがまだ生成されていない場合は、m_enemyCountが設定されているか確認する
    //ウィジェットを生成して、グローバル変数に保持する
    TSubclassOf<UEnemyCount> WidgetClass = GetDefault<UZombieAttackUISettings>()->GetEnemyCountWidgetClass();
    if (!WidgetClass)
    {
        WidgetClass = m_enemyCount;
        if (!WidgetClass)
        {
            WidgetClass = UEnemyCount::StaticClass();
        }
    }
    m_pEnemyCountWidget = CreateWidget<UEnemyCount>(GetWorld(), WidgetClass);
    if (m_pEnemyCountWidget)
    {
        m_pEnemyCountWidget->AddToViewport(10);
        m_pEnemyCountWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        g_enemyCountWidget = m_pEnemyCountWidget;
        UpdateEnemyCountWidget();
    }
}

//敵の残り数をウィジェットに反映する関数
void ASpawnEnemies::NotifyGoalActivated(UWorld* _pWorld)
{
    if (_pWorld && g_enemyCountWidget.IsValid() && g_enemyCountWidget->GetWorld() == _pWorld)
    {
        g_enemyCountWidget->ShowGoalReady();
    }
}

//生存している敵数をHUDへ反映します。
void ASpawnEnemies::UpdateEnemyCountWidget()
{
    //ウィジェットがまだ生成されていない場合は、グローバル変数から取得する
    if (g_enemyCountWidget.IsValid() && g_enemyCountWidget->GetWorld() == GetWorld())
    {
        m_pEnemyCountWidget = g_enemyCountWidget.Get();
    }

    //ウィジェットが有効な場合は、敵の残り数を更新する
    if (m_pEnemyCountWidget)
    {
        m_pEnemyCountWidget->UpdateEnemyCountUI(GetMissionRemainingEnemyCount());
    }
}

//ミッションに残っている敵数を取得して呼び出し元へ返します。
int32 ASpawnEnemies::GetMissionRemainingEnemyCount() const
{
    //ワールドを返します。
    const UWorld* world = GetWorld();
    if (!world) { return m_remainingEnemies; }
    int32 totalRemainingEnemies = 0;
    //「TActorIterator<ASpawnEnemies> iterator(world); iterator; ++iterator」で列挙される各要素へ、ループ本体の判定と更新を適用します。
    for (TActorIterator<ASpawnEnemies> iterator(world); iterator; ++iterator)
    {
        const ASpawnEnemies* spawnActor = *iterator;
        if (IsValid(spawnActor) && spawnActor->m_bIsActivated)
        {
            totalRemainingEnemies += spawnActor->m_remainingEnemies;
        }
    }
    return totalRemainingEnemies;
}

//敵クラスの配列から有効なクラスの数をカウントする関数
int32 ASpawnEnemies::CountValidEnemyClasses(const TArray<TSubclassOf<AEnemyChara>>& _enemyClasses) const
{
    //有効なクラスの数をカウントする変数を初期化
    int32 validCount = 0;

    //敵クラスの配列をループして、有効なクラスの数をカウントする
    for (const TSubclassOf<AEnemyChara>& enemyClass : _enemyClasses)
    {
        //有効なクラスがnullptrでない場合は、カウントを増やす
        if (enemyClass != nullptr)
        {
            ++validCount;
        }
    }
    //有効なクラスの数を返す
    return validCount;
}

//全Waveを一度に生成する関数
void ASpawnEnemies::SpawnAllWaves(bool _bFreezeAfterSpawn)
{
    //ゲームオーバー時はスポーンを開始しない
    if (m_bIsGameOver) { return; }

    //ウィジェットを確実に生成する
    EnsureEnemyCountWidget();

    //全Waveを一度に生成するための準備
    m_spawnedEnemies.Empty();
    m_remainingEnemies = 0;
    m_currentWaveIndex = -1;
    UpdateEnemyCountWidget();

    //Waveデータの数を取得する
    const int32 waveCount = FMath::Min(m_waves.Num(), FMath::Max(0, m_maxWaveIndex));
    if (waveCount <= 0)
    {
        return;
    }

    //Waveデータをループして、各Waveの敵を生成する
    for (int32 waveIndex = 0; waveIndex < waveCount; ++waveIndex)
    {
        //現在のWaveインデックスを更新する
        m_currentWaveIndex = waveIndex;

        //Waveデータを取得する
        const FWaveEnemyData& wave = m_waves[waveIndex];
        const TArray<TSubclassOf<AEnemyChara>>& spawnClasses =
            (CountValidEnemyClasses(wave.m_waveEnemyClasses) > 0) ? wave.m_waveEnemyClasses : m_enemyClasses;

        //Waveの敵を生成する
        SpawnWaveEnemy(wave.m_enemiesToSpawn, spawnClasses);

        //Waveのボスクラスが設定されている場合は、ボスを生成する
        if (wave.m_bossClass)
        {
            SpawnBoss(wave.m_bossClass);
        }
    }

    //全Waveを生成した後、敵のAIと移動を止めるかどうかで処理を分ける
    if (_bFreezeAfterSpawn)
    {
        SetAllSpawnedEnemyGameplayEnabled(false);
    }
    else
    {
        SetAllSpawnedEnemyGameplayEnabled(true);
        m_bEnemiesReleased = true;
    }

    //ウィジェットを更新する
    UpdateEnemyCountWidget();

    //全Waveを一度に生成したことをログに出力する

    //敵が一度も生成されなかった場合は、エラーログを出力する
}

//次のWaveを開始する関数
void ASpawnEnemies::StartNextWave()
{
    //ゲームオーバー時はスポーンを開始しない
    if (m_bIsGameOver) { return; }

    //ウィジェットを確実に生成する
    EnsureEnemyCountWidget();

    //現在のWaveインデックスを更新する
    ++m_currentWaveIndex;

    //Waveデータが存在しない場合、または最大Waveインデックスを超えた場合は、ログを出力して終了する
    if (m_currentWaveIndex >= m_waves.Num() || m_currentWaveIndex >= m_maxWaveIndex)
    {
        return;
    }

    //現在のWaveの敵数をリセットする
    m_remainingEnemies = 0;
    UpdateEnemyCountWidget();

    //Waveデータを取得する
    const FWaveEnemyData& wave = m_waves[m_currentWaveIndex];
    const TArray<TSubclassOf<AEnemyChara>>& spawnClasses =
        (CountValidEnemyClasses(wave.m_waveEnemyClasses) > 0) ? wave.m_waveEnemyClasses : m_enemyClasses;

    //Waveの敵を生成する
    SpawnWaveEnemy(wave.m_enemiesToSpawn, spawnClasses);

    //Waveのボスクラスが設定されている場合は、ボスを生成する
    if (wave.m_bossClass)
    {
        SpawnBoss(wave.m_bossClass);
    }

    //ウィジェットを更新する
    UpdateEnemyCountWidget();

    //Waveの開始をログに出力する

    //敵が一度も生成されなかった場合は、エラーログを出力する
    if (m_remainingEnemies <= 0)
    {
        return;
    }
}

//Waveの終了処理を行う関数
void ASpawnEnemies::EndWave()
{
    //ゲームオーバー時はWaveの終了処理を行わない
    if (m_bIsGameOver) { return; }

    //次のWaveが存在するかどうかを判定する
    const int32 nextIndex = m_currentWaveIndex + 1;
    //判定結果を後の処理で使えるように記録します。
    const bool bHasNextWave = (nextIndex < m_waves.Num()) && (nextIndex < m_maxWaveIndex);
    if (bHasNextWave)
    {
        //ゼロ秒はTimerの取消扱いになるため、即時出現の設定でも次の更新で進める。
        const float delay = FMath::Max(0.01f, m_waves[m_currentWaveIndex].m_spawnInterval);
        GetWorld()->GetTimerManager().SetTimer(m_waveTimerHandle, this, &ASpawnEnemies::StartNextWave, delay, false);
        return;
    }

    //Waveがすべて終了した場合、次のSpawnerをアクティブ化する
    for (ASpawnEnemies* nextSpawn : m_nextSpawnActors)
    {
        if (nextSpawn && !nextSpawn->m_bIsActivated)
        {
            nextSpawn->ActivateSpawn();
        }
    }
}

//エネミーを生成する
void ASpawnEnemies::SpawnWaveEnemy(int32 _count, const TArray<TSubclassOf<AEnemyChara>>& _enemyClasses)
{
    //カウントが０以下の場合は何もしない
    if (_count <= 0) { return; }

    //敵クラスの配列が空の場合はエラーログを出力して終了する
    if (CountValidEnemyClasses(_enemyClasses) <= 0)
    {
        return;
    }

    //ワールドを取得する
    UWorld* world = GetWorld();
    if (!world) { return; }

    //ナビゲーションシステムを取得する
    UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(world);

    //指定された数だけ敵を生成するループ
    for (int32 index = 0; index < _count; ++index)
    {
        //ランダムに敵クラスを選択する
        TSubclassOf<AEnemyChara> selectedClass = nullptr;

        //ランダムに敵クラスを選択する際、最大10回まで試行する
        for (int32 attempt = 0; attempt < 10; ++attempt)
        {
            //敵クラスの配列からランダムにインデックスを選択する
            const int32 classIndex = FMath::RandRange(0, _enemyClasses.Num() - 1);
            if (_enemyClasses[classIndex])
            {
                //有効な敵クラスが見つかった場合は、selectedClassに設定してループを抜ける
                selectedClass = _enemyClasses[classIndex];
                break;
            }
        }

        //有効な敵クラスが見つからなかった場合は、次のループに進む
        if (!selectedClass)
        {
            continue;
        }

        //ナビゲーションシステムを使用して、指定された範囲内でランダムなスポーンポイントを取得する
        const FVector desiredPosition = GetRandomSpawnPointInRect();
        FNavLocation navLocation;
        //探索対象を有効な範囲内で発見できたかを示します。
        const bool bFound = navSystem && navSystem->GetRandomReachablePointInRadius(desiredPosition, m_spawnRadius, navLocation);

        //BeginPlay直後はNavMeshがまだ準備中の場合があります。
        //その場合も敵生成自体は中止せず、矩形内の位置を地面へ投影します。
        FVector spawnPosition = bFound ? navLocation.Location : desiredPosition;

        //敵のカプセルコンポーネントの半径を取得する
        float capsuleHalfHeight = 0.0f;
        if (const ACharacter* defaultCharacter = Cast<ACharacter>(selectedClass->GetDefaultObject()))
        {
            //カプセルコンポーネントが存在する場合は、スケールされたカプセルの半径を取得する
            if (defaultCharacter->GetCapsuleComponent())
            {
                capsuleHalfHeight = defaultCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
            }
        }

        //地面の高さを取得するためにラインをトレースする
        FHitResult groundHit;
        FCollisionQueryParams queryParams;
        queryParams.bTraceComplex = true;
        queryParams.AddIgnoredActor(this);

        //ナビメッシュの結果は歩行可能な床を示しているため、その高さを優先する。
        //再トレースすると街灯などを地面と誤認し、敵が装飾物の上に生成される場合がある。
        if (bFound)
        {
            spawnPosition.Z = navLocation.Location.Z + capsuleHalfHeight;
        }
        else if (world->LineTraceSingleByChannel(groundHit, spawnPosition + FVector(0.0f, 0.0f, 300.0f), spawnPosition - FVector(0.0f, 0.0f, 500.0f),
                                                 ECC_WorldStatic, queryParams))
        {
            spawnPosition.Z = groundHit.Location.Z + capsuleHalfHeight;
        }

        //敵をスポーンするためのパラメータを設定する
        FActorSpawnParameters spawnParams;
        spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        //敵をスポーンする
        AEnemyChara* spawnedEnemy = world->SpawnActor<AEnemyChara>(selectedClass, spawnPosition, FRotator::ZeroRotator, spawnParams);

        //スポーンに成功した場合は、敵のコントローラーを生成し、リストに追加する
        if (spawnedEnemy)
        {
            if (!spawnedEnemy->GetController())
            {
                spawnedEnemy->SpawnDefaultController();
            }

            //敵が破壊されたときに呼ばれるデリゲートを登録する
            m_spawnedEnemies.Add(spawnedEnemy);
            spawnedEnemy->OnDestroyed.AddDynamic(this, &ASpawnEnemies::OnEnemyDeath);
            ++m_remainingEnemies;
            UpdateEnemyCountWidget();
        }
    }
}

//ボスをスポーンする関数
void ASpawnEnemies::SpawnBoss(TSubclassOf<AEnemyChara> _bossClass)
{
    //ボスクラスが有効でない場合は何もしない
    if (!_bossClass) { return; }

    //ワールドを取得する
    UWorld* world = GetWorld();
    if (!world) { return; }

    //ナビゲーションシステムを取得する
    UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(world);

    //ボスのカプセルコンポーネントの半径を取得する
    float capsuleHalfHeight = 0.0f;
    //続けて「defaultCharacter->GetCapsuleComponent()」を判定します。
    if (const ACharacter* defaultCharacter = Cast<ACharacter>(_bossClass->GetDefaultObject()))
    {
        if (defaultCharacter->GetCapsuleComponent())
        {
            capsuleHalfHeight = defaultCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        }
    }

    //ボスのスポーン候補位置を格納する配列を作成する
    TArray<FVector> candidates;

    //ナビゲーションシステムが有効な場合は、指定された範囲内でランダムなスポーンポイントを取得する
    if (navSystem)
    {
        //「int32 index = 0; index < 5; ++index」で列挙される各要素へ、ループ本体の判定と更新を適用します。
        for (int32 index = 0; index < 5; ++index)
        {
            FNavLocation navLocation;
            if (navSystem->GetRandomReachablePointInRadius(GetActorLocation(), m_spawnRadius * 0.5f, navLocation))
            {
                candidates.Add(navLocation.Location);
            }
        }
    }
    //NavMeshがないマップでも生成できるよう、Spawner位置を最後の候補にする。
    candidates.Add(GetActorLocation());

    //スポーン候補位置をループして、ボスをスポーンする
    for (FVector candidate : candidates)
    {
        //地面の高さを取得するためにラインをトレースする
        FHitResult groundHit;
        FCollisionQueryParams queryParams;
        queryParams.AddIgnoredActor(this);

        //NavMeshの床高さを優先し、街灯や小物の上への誤生成を防止する。
        FNavLocation projectedLocation;
        //生成候補位置をNavMesh上へ補正できたかを示します。
        const bool bProjectedToNavigation =
            navSystem && navSystem->ProjectPointToNavigation(candidate, projectedLocation, FVector(250.0f, 250.0f, 500.0f));
        if (bProjectedToNavigation)
        {
            candidate = projectedLocation.Location;
            candidate.Z += capsuleHalfHeight;
        }
        else if (world->LineTraceSingleByChannel(groundHit, candidate + FVector(0.0f, 0.0f, 300.0f), candidate - FVector(0.0f, 0.0f, 500.0f),
                                                 ECC_WorldStatic, queryParams))
        {
            candidate.Z = groundHit.Location.Z + capsuleHalfHeight;
        }
        else
        {
            candidate.Z += capsuleHalfHeight;
        }

        //ボスをスポーンするためのパラメータを設定する
        FActorSpawnParameters spawnParams;
        spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        //ボスをスポーンする
        AEnemyChara* boss = world->SpawnActor<AEnemyChara>(_bossClass, candidate, FRotator::ZeroRotator, spawnParams);
        if (boss)
        {
            if (!boss->GetController())
            {
                boss->SpawnDefaultController();
            }

            //ボスが破壊されたときに呼ばれるデリゲートを登録する
            m_spawnedEnemies.Add(boss);
            boss->OnDestroyed.AddDynamic(this, &ASpawnEnemies::OnEnemyDeath);
            ++m_remainingEnemies;
            UpdateEnemyCountWidget();
            return;
        }
    }
}

//ランダムなスポーンポイントを矩形内で取得する関数
FVector ASpawnEnemies::GetRandomSpawnPointInRect() const
{
    //矩形内のランダムなX座標とY座標を生成する
    const float randomX = FMath::RandRange(-m_spawnHalfExtentX, m_spawnHalfExtentX);
    const float randomY = FMath::RandRange(-m_spawnHalfExtentY, m_spawnHalfExtentY);
    //GetActorLocationは、呼び出し元が必要とする対象または計算結果を返します。
    return GetActorLocation() + GetActorRotation().RotateVector(FVector(randomX, randomY, 0.0f));
}

//敵ゲームプレイ有効をゲーム内の対象へ反映します。
void ASpawnEnemies::SetEnemyGameplayEnabled(AEnemyChara* _enemy, bool _bEnabled)
{
    //敵が有効でない場合は何もしない
    if (!IsValid(_enemy)) { return; }

    //敵のTickを有効または無効にする
    _enemy->SetActorTickEnabled(_bEnabled);

    //敵の移動コンポーネントを取得して、移動モードを設定する
    if (ACharacter* character = Cast<ACharacter>(_enemy))
    {
        if (UCharacterMovementComponent* movement = character->GetCharacterMovement())
        {
            //移動モードを有効または無効にする
            if (_bEnabled)
            {
                movement->SetMovementMode(MOVE_Walking);
            }
            else
            {
                movement->StopMovementImmediately();
                movement->DisableMovement();
            }
        }

        //敵のコントローラーが存在しない場合は、デフォルトのコントローラーを生成する
        if (_bEnabled && !character->GetController())
        {
            character->SpawnDefaultController();
        }
    }

    //敵のAIコントローラーを取得して、Tickを有効または無効にする
    AAIController* aiController = Cast<AAIController>(_enemy->GetController());
    if (aiController)
    {
        if (_bEnabled)
        {
            aiController->SetActorTickEnabled(true);
        }
        else
        {
            aiController->StopMovement();
            aiController->SetActorTickEnabled(false);
        }
    }
}

//すべてのスポーン済み敵のゲームプレイを有効または無効にする関数
void ASpawnEnemies::SetAllSpawnedEnemyGameplayEnabled(bool _bEnabled)
{
    for (AEnemyChara* enemy : m_spawnedEnemies)
    {
        SetEnemyGameplayEnabled(enemy, _bEnabled);
    }
}

//敵が破壊されたときに呼ばれる関数
void ASpawnEnemies::OnEnemyDeath(AActor* _destroyedActor)
{
    //敗北時の一括削除を撃破として数えず、削除中の配列変更と誤ったクリア判定を防ぐ。
    if (m_bIsGameOver) { return; }
    //破壊されたアクターが敵キャラクターであるかを確認する
    AEnemyChara* deadEnemy = Cast<AEnemyChara>(_destroyedActor);
    if (!deadEnemy) { return; }

    //破壊された敵をスポーン済みリストから削除し、残りの敵数を更新する
    //同じ破棄通知や、このSpawnerが生成していない敵では残り数を減らさない。
    if (m_spawnedEnemies.Remove(deadEnemy) == 0) { return; }
    m_remainingEnemies = FMath::Max(0, m_remainingEnemies - 1);
    UpdateEnemyCountWidget();

    //全Waveを一度に生成するモードの場合、残りの敵が0になったらゴールをアクティブ化する
    if (m_bSpawnAllWavesAtOnce)
    {
        if (m_remainingEnemies <= 0)
        {
            TryActivateGoalAfterAllEnemiesDefeated();
        }
        return;
    }

    //Waveモードの場合、ボスが倒されたらゴールをアクティブ化する
    if (deadEnemy->IsA<AFinalBossChara>())
    {
        if (m_pGoalActor)
        {
            m_pGoalActor->ActivateGoal();
        }
        else
        {
            HandleGameClear();
        }
        return;
    }

    //Waveモードの場合、残りの敵が0になったらWaveを終了する
    if (m_remainingEnemies <= 0)
    {
        EndWave();
    }
}

//全Waveを一度に生成するモードで、すべての敵が倒された後にゴールをアクティブ化する関数
void ASpawnEnemies::TryActivateGoalAfterAllEnemiesDefeated()
{
    //敗北が決まった後に別の敵の破棄通知が来ても、ゴールを開かない。
    if (m_bIsGameOver) { return; }
    //ワールドを返します。
    UWorld* world = GetWorld();
    if (!world) { return; }
    AGoalActor* missionGoal = nullptr;
    //「TActorIterator<ASpawnEnemies> iterator(world); iterator; ++iterator」で列挙される各要素へ、ループ本体の判定と更新を適用します。
    for (TActorIterator<ASpawnEnemies> iterator(world); iterator; ++iterator)
    {
        ASpawnEnemies* spawnActor = *iterator;
        if (!IsValid(spawnActor) || !spawnActor->m_bIsActivated)
        {
            continue;
        }
        if (spawnActor->m_remainingEnemies > 0) { return; }
        if (!missionGoal && IsValid(spawnActor->m_pGoalActor))
        {
            missionGoal = spawnActor->m_pGoalActor;
        }
    }
    if (missionGoal)
    {
        missionGoal->ActivateGoal();
        return;
    }

    HandleGameClear();
}

//ゲームクリア時に呼ばれる関数
void ASpawnEnemies::HandleGameClear()
{
    //クリアとゲームオーバーの両方から遷移要求が出るのを防ぐ。
    if (m_bIsGameOver) { return; }
    //ゲームクリア時の処理を行う
    m_bIsGameOver = true;
    GetWorld()->GetTimerManager().ClearTimer(m_waveTimerHandle);

    //ワールドを返します。
    if (APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        playerController->SetPause(true);
    }

    //3秒後にゲームクリア画面に遷移する
    UGameplayStatics::OpenLevel(GetWorld(), FName(TEXT("GameClear")));
}

//ゲームオーバー時に呼ばれる関数
void ASpawnEnemies::HandleGameOver(ABaseCharacter* _deadCharacter)
{
    //死亡通知が重複しても、削除中の敵配列へ再入しない。
    if (m_bIsGameOver) { return; }
    //ゲームオーバー時の処理を行う
    m_bIsGameOver = true;
    GetWorld()->GetTimerManager().ClearTimer(m_waveTimerHandle);

    //すべてのスポーン済み敵を破棄する
    for (AEnemyChara* enemy : m_spawnedEnemies)
    {
        if (enemy && !enemy->IsPendingKillPending())
        {
            enemy->Destroy();
        }
    }

    //すべてのスポーン済み敵を破棄した後、リストをクリアする
    m_spawnedEnemies.Empty();
    m_remainingEnemies = 0;
    UpdateEnemyCountWidget();

    //操作キャラクターは死亡アニメの完了後に自分で遷移するため、固定三秒で演出を切らない。
    if (Cast<APlayerChara>(_deadCharacter)) { return; }

    //独自のBaseCharacterを使う派生ゲームだけ、三秒後の代替遷移を予約する。
    GetWorld()->GetTimerManager().SetTimer(m_gameOverTimerHandle,
                                           FTimerDelegate::CreateWeakLambda(this,
                                               [this]()
                                               {
                                                   if (GetWorld())
                                                   {
                                                       //Levelへ安全に遷移します。
                                                       UGameplayStatics::OpenLevel(GetWorld(), FName(TEXT("GameOver")));
                                                   }
                                               }),
                                           3.0f, false);
}
