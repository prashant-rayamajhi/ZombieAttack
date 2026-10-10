#include "IntroCutsceneDirector.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "ZombieAttack/Enemy/SpawnEnemies.h"
#include "ZombieAttack/Enemy/EnemyChara.h"
#include "ZombieAttack/Enemy/BossChara/FinalBossChara.h"
#include "ZombieAttack/Enemy/BossChara/MidBossChara.h"
#include "ZombieAttack/Goal/GoalActor.h"
#include "ZombieAttack/Player/PlayerChara.h"
#include "ZombieAttack/UI/PlayerUI/CombatCrosshairWidget.h"
#include "ZombieAttack/UI/EnemyUI/EnemyCount.h"

//コンストラクタ
AIntroCutsceneDirector::AIntroCutsceneDirector()
    : m_moveDurationPerTarget(2.25f), m_holdDurationAtTarget(0.45f), m_returnBlendTime(0.8f), m_endHoldDuration(0.35f), m_fadeOutDuration(0.06f),
      m_hiddenTravelDuration(0.10f), m_fadeInDuration(0.12f), m_minimumShotHoldDuration(0.85f), m_cameraFOV(50.0f), m_bUseSmoothEase(true),
      m_bAvoidCameraObstructions(true), m_bAutoFrameMissionSubjects(true), m_obstructionProbeRadius(60.0f), m_obstructionLift(180.0f),
      m_bPlayOnBeginPlay(true), m_bDisablePlayerControlDuringCutscene(true), m_runtimeCamera(nullptr),
      m_cinematicFillLight(nullptr), m_currentTargetIndex(0), m_nextTargetIndex(1), m_moveElapsedTime(0.0f), m_holdElapsedTime(0.0f),
      m_endHoldElapsedTime(0.0f), m_bCutscenePlaying(false), m_bMovingToNextTarget(false), m_bWaitingAtTarget(false), m_bEndingCutscene(false)
{
    //Tickを有効化
    PrimaryActorTick.bCanEverTick = true;

    //夜間でも紹介対象の輪郭が読める、影を生成しない弱い撮影用ライトです。
    m_cinematicFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CinematicFillLight"));
    m_cinematicFillLight->SetMobility(EComponentMobility::Movable);
    m_cinematicFillLight->SetIntensity(22000.0f);
    m_cinematicFillLight->SetAttenuationRadius(1400.0f);
    m_cinematicFillLight->SetLightColor(FLinearColor(0.55f, 0.68f, 1.0f));
    m_cinematicFillLight->SetCastShadows(false);
    m_cinematicFillLight->SetVisibility(false);
}

//ゲーム開始時に呼ばれる関数
void AIntroCutsceneDirector::BeginPlay()
{
    //親クラスのBeginPlayを呼び出す
    Super::BeginPlay();

    //プレイヤーコントローラーとポーンを取得
    m_playerController = UGameplayStatics::GetPlayerController(this, 0);
    if (m_playerController.IsValid())
    {
        m_playerPawn = m_playerController->GetPawn();
    }

    DiscoverSpawnActors();
    BuildMissionSubjectShots();

    //BeginPlay時に自動でカットシーンを開始する場合は、StartCutsceneを呼び出す
    if (m_bPlayOnBeginPlay)
    {
        StartCutscene();
    }
}

//ミッションSubjectShotsを作成します。
void AIntroCutsceneDirector::BuildMissionSubjectShots()
{
    //ワールドを返します。
    UWorld* world = GetWorld();
    if (!m_bAutoFrameMissionSubjects || !world) { return; }
    AGoalActor* goal = nullptr;
    AFinalBossChara* finalBoss = nullptr;
    AMidBossChara* midBoss = nullptr;
    AEnemyChara* regularEnemy = nullptr;
    for (TActorIterator<AGoalActor> iterator(world); iterator; ++iterator)
    {
        if (IsValid(*iterator))
        {
            goal = *iterator;
            break;
        }
    }

    //「TActorIterator<AEnemyChara> iterator(world); iterator; ++iterator」で列挙される各要素へ、ループ本体の判定と更新を適用します。
    for (TActorIterator<AEnemyChara> iterator(world); iterator; ++iterator)
    {
        //敵を保持します。
        AEnemyChara* enemy = *iterator;
        if (!IsValid(enemy))
        {
            continue;
        }
        if (!finalBoss)
        {
            finalBoss = Cast<AFinalBossChara>(enemy);
        }
        if (!midBoss)
        {
            midBoss = Cast<AMidBossChara>(enemy);
        }
        if (!regularEnemy && !enemy->IsA<AFinalBossChara>() && !enemy->IsA<AMidBossChara>())
        {
            regularEnemy = enemy;
        }
    }

    //ステージの目的を先に示し、脅威の強い順に紹介して、
    //最後に操作するプレイヤーへ戻る5ショット構成です。
    TArray<AActor*> orderedSubjects;
    if (IsValid(goal))
    {
        orderedSubjects.Add(goal);
    }
    if (IsValid(finalBoss))
    {
        orderedSubjects.Add(finalBoss);
    }
    if (IsValid(midBoss))
    {
        orderedSubjects.Add(midBoss);
    }
    if (IsValid(regularEnemy))
    {
        orderedSubjects.Add(regularEnemy);
    }
    if (m_playerPawn.IsValid())
    {
        orderedSubjects.Add(m_playerPawn.Get());
    }
    if (orderedSubjects.IsEmpty()) { return; }

    m_viewTargets.SetNum(orderedSubjects.Num());
    m_lookAtTargets.SetNum(orderedSubjects.Num());
    //「int32 index = 0; index < orderedSubjects.Num(); ++index」で列挙される各要素へ、ループ本体の判定と更新を適用します。
    for (int32 index = 0; index < orderedSubjects.Num(); ++index)
    {
        m_viewTargets[index] = orderedSubjects[index];
        m_lookAtTargets[index] = orderedSubjects[index];
    }
}

//SpawnActorsをレベル内から収集します。
void AIntroCutsceneDirector::DiscoverSpawnActors()
{
    //ワールドを返します。
    UWorld* world = GetWorld();
    if (!world) { return; }

    //「TActorIterator<ASpawnEnemies> iterator(world); iterator; ++iterator」で列挙される各要素へ、ループ本体の判定と更新を適用します。
    for (TActorIterator<ASpawnEnemies> iterator(world); iterator; ++iterator)
    {
        ASpawnEnemies* spawnActor = *iterator;
        if (IsValid(spawnActor))
        {
            m_spawnActorsToActivate.AddUnique(spawnActor);
        }
    }

    m_spawnActorsToActivate.Sort([](const ASpawnEnemies& _left, const ASpawnEnemies& _right) { return _left.GetName() < _right.GetName(); });

}

//毎フレーム呼ばれる関数
void AIntroCutsceneDirector::Tick(float _deltaTime)
{
    //親クラスのTickを呼び出す
    Super::Tick(_deltaTime);

    UpdateGameplayHUDReveal(_deltaTime);

    //カットシーンが再生中でない場合は処理を終了
    if (!m_bCutscenePlaying) { return; }

    //カメラの更新処理を行う
    //HUDの生成順に左右されず、映像中は戦闘UIを一切表示しない
    SetGameplayHUDHidden();
    UpdateCinematicCamera(_deltaTime);
}

//カットシーンを開始する関数
void AIntroCutsceneDirector::StartCutscene()
{
    //プレイヤーコントローラーが有効でない場合はカットシーンを終了する
    if (!m_playerController.IsValid())
    {
        FinishCutscene();
        return;
    }

    //カットシーンの通過地点が1つもない場合、または最初の通過地点が有効でない場合はカットシーンを終了する
    if (m_viewTargets.Num() <= 0 || !IsValidViewTargetIndex(0))
    {
        FinishCutscene();
        return;
    }

    //RuntimeCameraが有効でない場合は生成する
    CreateRuntimeCameraIfNeeded();
    if (!IsValid(m_runtimeCamera))
    {
        FinishCutscene();
        return;
    }

    //カットシーンの状態を初期化する
    m_bCutscenePlaying = true;
    m_bEndingCutscene = false;
    m_currentTargetIndex = 0;
    m_nextTargetIndex = 1;
    m_moveElapsedTime = 0.0f;
    m_holdElapsedTime = 0.0f;
    m_endHoldElapsedTime = 0.0f;

    //カットシーン映像にはクロスヘアを含む戦闘HUDを重ねない
    m_bHUDRevealPlaying = false;
    m_hudRevealEntries.Reset();
    SetGameplayHUDHidden();

    //プレイヤーの操作を無効化する
    if (m_bDisablePlayerControlDuringCutscene)
    {
        SetPlayerControlEnabled(false);
    }

    //カットシーン開始時に敵を生成する場合は、各SpawnerのPrepareAllEnemiesForIntro()を呼び出す
    for (ASpawnEnemies* spawnActor : m_spawnActorsToActivate)
    {
        if (IsValid(spawnActor))
        {
            spawnActor->PrepareAllEnemiesForIntro();
        }
    }
    BuildMissionSubjectShots();

    //RuntimeCameraを最初の通過地点にスナップする
    SnapRuntimeCameraToTarget(0);

    //プレイヤーコントローラーのViewTargetをRuntimeCameraに切り替える
    m_playerController->SetViewTarget(m_runtimeCamera);


    //最初の通過地点に到達した状態で待機する
    if (m_viewTargets.Num() > 1)
    {
        m_bWaitingAtTarget = true;
        m_bMovingToNextTarget = false;
        //最初のショットだけStreaming完了を待ちます。
        m_holdElapsedTime = -0.8f;
    }
    else
    {
        m_bEndingCutscene = true;
    }
}

//カットシーンを終了する関数
void AIntroCutsceneDirector::CreateRuntimeCameraIfNeeded()
{
    //すでにRuntimeCameraが有効な場合は処理を終了する
    if (IsValid(m_runtimeCamera)) { return; }

    //ワールドを取得する
    UWorld* world = GetWorld();
    if (!world) { return; }

    //RuntimeCameraを生成するためのパラメータを設定する
    FActorSpawnParameters spawnParams;
    spawnParams.Owner = this;
    spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    //RuntimeCameraを生成する
    m_runtimeCamera = world->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), GetActorLocation(), GetActorRotation(), spawnParams);

    //RuntimeCameraのFOVを設定する
    if (m_runtimeCamera && m_runtimeCamera->GetCameraComponent())
    {
        //カメラコンポーネントを返します。
        UCameraComponent* cameraComponent = m_runtimeCamera->GetCameraComponent();
        cameraComponent->SetFieldOfView(m_cameraFOV);
        cameraComponent->PostProcessBlendWeight = 1.0f;
        cameraComponent->PostProcessSettings.bOverride_AutoExposureBias = true;
        cameraComponent->PostProcessSettings.AutoExposureBias = -0.35f;
        cameraComponent->PostProcessSettings.bOverride_VignetteIntensity = true;
        cameraComponent->PostProcessSettings.VignetteIntensity = 0.18f;
        cameraComponent->PostProcessSettings.bOverride_MotionBlurAmount = true;
        cameraComponent->PostProcessSettings.MotionBlurAmount = 0.0f;
    }
    if (m_runtimeCamera && m_cinematicFillLight)
    {
        m_cinematicFillLight->AttachToComponent(m_runtimeCamera->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        m_cinematicFillLight->SetRelativeLocation(FVector(80.0f, 0.0f, 20.0f));
        m_cinematicFillLight->SetVisibility(true);
    }
}

//プレイヤーの操作を有効化または無効化する関数
bool AIntroCutsceneDirector::IsValidViewTargetIndex(int32 _targetIndex) const
{
    return m_viewTargets.IsValidIndex(_targetIndex) && IsValid(m_viewTargets[_targetIndex]);
}

//指定されたインデックスの通過地点のTransformを取得する関数
FTransform AIntroCutsceneDirector::GetViewTargetTransform(int32 _targetIndex) const
{
    //指定されたインデックスが有効でない場合は、Identity Transformを返す
    if (!IsValidViewTargetIndex(_targetIndex)) { return FTransform::Identity; }

    //最後は普段の肩越し視点へつなぎ、操作開始時に別方向へ飛ばないようにする。
    if (m_viewTargets[_targetIndex] == m_playerPawn.Get())
    {
        if (UCameraComponent* camera = m_playerPawn->FindComponentByClass<UCameraComponent>()) { return camera->GetComponentTransform(); }
    }
    //編集済みの構図を基準にし、遮蔽物が被写体を隠す場合だけカメラ位置を補正します。
    AActor* viewTarget = m_viewTargets[_targetIndex];
    FTransform targetTransform = viewTarget->GetActorTransform();
    if (m_bAutoFrameMissionSubjects && m_lookAtTargets.IsValidIndex(_targetIndex) && IsValid(m_lookAtTargets[_targetIndex]))
    {
        const float focusHeight = m_lookAtTargets[_targetIndex]->IsA<AGoalActor>() ? 175.0f : 20.0f;
        const FVector subjectFocus = m_lookAtTargets[_targetIndex]->GetActorLocation() + FVector(0.0f, 0.0f, focusHeight);
        FVector subjectToPlayer =
            m_playerPawn.IsValid() ? m_playerPawn->GetActorLocation() - subjectFocus : -m_lookAtTargets[_targetIndex]->GetActorForwardVector();
        subjectToPlayer.Z = 0.0f;
        if (subjectToPlayer.IsNearlyZero())
        {
            subjectToPlayer = FVector::BackwardVector;
        }

        //既存CameraPointが樹木や街灯の内部に置かれていても使用せず、
        //プレイヤー側から敵集団を見渡す5種類の安全な画角を構成します。
        static const float ShotYawOffsets[] = {0.0f, 58.0f, -72.0f, -28.0f, 90.0f};
        const float yawOffset = ShotYawOffsets[_targetIndex % UE_ARRAY_COUNT(ShotYawOffsets)];
        const FVector shotDirection = subjectToPlayer.GetSafeNormal().RotateAngleAxis(yawOffset, FVector::UpVector);
        static const float ShotDistances[] = {850.0f, 620.0f, 560.0f, 650.0f, 500.0f};
        static const float ShotHeights[] = {160.0f, 80.0f, 70.0f, 90.0f, 60.0f};
        const float shotDistance = ShotDistances[_targetIndex % UE_ARRAY_COUNT(ShotDistances)];
        const float shotHeight = ShotHeights[_targetIndex % UE_ARRAY_COUNT(ShotHeights)];
        targetTransform.SetLocation(subjectFocus + shotDirection * shotDistance + FVector(0.0f, 0.0f, shotHeight));
    }

    targetTransform.SetLocation(ResolveObstructionSafeLocation(targetTransform.GetLocation(), _targetIndex));

    //LookAt対象が有効であれば、カメラをLookAtするように回転を設定する
    const FRotator lookAtRotation = CalculateLookAtRotation(targetTransform.GetLocation(), _targetIndex);

    //LookAt対象が有効であれば、カメラをLookAtするように回転を設定する
    targetTransform.SetRotation(lookAtRotation.Quaternion());
    return targetTransform;
}

//遮蔽物を貫通しないカットシーン用カメラ位置を求めます。
FVector AIntroCutsceneDirector::ResolveObstructionSafeLocation(const FVector& _desiredLocation, int32 _targetIndex) const
{
    if (!m_bAvoidCameraObstructions || !GetWorld() || !m_lookAtTargets.IsValidIndex(_targetIndex) || !IsValid(m_lookAtTargets[_targetIndex]))
    {
        return _desiredLocation;
    }
    AActor* subject = m_lookAtTargets[_targetIndex];
    const float focusHeight = subject->IsA<AGoalActor>() ? 175.0f : 20.0f;
    const FVector subjectLocation = subject->GetActorLocation() + FVector(0.0f, 0.0f, focusHeight);
    FCollisionQueryParams queryParams(SCENE_QUERY_STAT(IntroCutsceneObstruction), false, this);
    queryParams.AddIgnoredActor(subject);
    queryParams.AddIgnoredActor(m_viewTargets[_targetIndex]);
    if (m_playerPawn.IsValid())
    {
        queryParams.AddIgnoredActor(m_playerPawn.Get());
    }

    const auto isCameraPathClear = [this, &queryParams, &subjectLocation](const FVector& _candidate)
    {
        FHitResult obstruction;
        //ワールドを返します。
        const bool bForegroundBlocked =
            GetWorld()->SweepSingleByChannel(obstruction, subjectLocation, _candidate, FQuat::Identity, ECC_Visibility,
                                             FCollisionShape::MakeSphere(FMath::Max(10.0f, m_obstructionProbeRadius)), queryParams);
        if (bForegroundBlocked) { return false; }
        FVector backgroundDirection = subjectLocation - _candidate;
        backgroundDirection.Z = 0.0f;
        backgroundDirection.Normalize();
        FHitResult backgroundObstruction;
        return !GetWorld()->SweepSingleByChannel(backgroundObstruction, subjectLocation, subjectLocation + backgroundDirection * 240.0f,
                                                 FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(35.0f), queryParams);
    };
    if (isCameraPathClear(_desiredLocation)) { return _desiredLocation; }

    //編集済みの角度が遮られた場合も、被写体とのシネマティックな距離を維持します。
    //トレース衝突点への直接移動で極端な俯瞰接写にならないよう、複数の候補位置を試します。
    FVector horizontalOffset = _desiredLocation - subjectLocation;
    const float desiredHeight = FMath::Clamp(horizontalOffset.Z, 70.0f, 240.0f);
    horizontalOffset.Z = 0.0f;
    if (horizontalOffset.IsNearlyZero())
    {
        horizontalOffset = FVector::BackwardVector;
    }
    horizontalOffset = horizontalOffset.GetSafeNormal() * FMath::Max(500.0f, horizontalOffset.Size());

    static const float OrbitAngles[] = {35.0f, -35.0f, 70.0f, -70.0f, 110.0f, -110.0f, 180.0f};
    //「float orbitAngle : OrbitAngles」で列挙される各要素へ、ループ本体の判定と更新を適用します。
    for (float orbitAngle : OrbitAngles)
    {
        const FVector candidate =
            subjectLocation + horizontalOffset.RotateAngleAxis(orbitAngle, FVector::UpVector) + FVector(0.0f, 0.0f, desiredHeight);
        if (isCameraPathClear(candidate)) { return candidate; }
    }
    for (float orbitAngle : OrbitAngles)
    {
        const FVector candidate = subjectLocation + horizontalOffset.RotateAngleAxis(orbitAngle, FVector::UpVector) +
                                  FVector(0.0f, 0.0f, desiredHeight + m_obstructionLift);
        if (isCameraPathClear(candidate)) { return candidate; }
    }

    return subjectLocation + horizontalOffset.GetSafeNormal() * 950.0f + FVector(0.0f, 0.0f, desiredHeight + m_obstructionLift);
}

//指定されたインデックスのLookAt対象のRotationを取得する関数
FRotator AIntroCutsceneDirector::CalculateLookAtRotation(const FVector& _cameraLocation, int32 _targetIndex) const
{
    //LookAt対象が有効であれば、カメラをLookAtするように回転を設定する
    if (m_lookAtTargets.IsValidIndex(_targetIndex) && IsValid(m_lookAtTargets[_targetIndex]))
    {
        //LookAt対象の位置を取得する
        const AActor* subject = m_lookAtTargets[_targetIndex];
        const float focusHeight = subject->IsA<AGoalActor>() ? 175.0f : 20.0f;
        const FVector toTarget = subject->GetActorLocation() + FVector(0.0f, 0.0f, focusHeight) - _cameraLocation;

        //LookAt対象の位置がカメラの位置とほぼ同じ場合は、回転を設定しない
        if (!toTarget.IsNearlyZero()) { return toTarget.Rotation(); }
    }

    //LookAt対象が有効でない場合は、通過地点の回転をそのまま返す
    if (IsValidViewTargetIndex(_targetIndex)) { return m_viewTargets[_targetIndex]->GetActorRotation(); }

    //LookAt対象が有効でない場合は、Identity Rotationを返す
    return FRotator::ZeroRotator;
}

//RuntimeCameraを指定されたインデックスの通過地点にスナップする関数
void AIntroCutsceneDirector::SnapRuntimeCameraToTarget(int32 _targetIndex)
{
    //RuntimeCameraが有効でない場合、または指定されたインデックスの通過地点が有効でない場合は処理を終了する
    if (!IsValid(m_runtimeCamera) || !IsValidViewTargetIndex(_targetIndex)) { return; }

    //指定されたインデックスの通過地点のTransformを取得する
    const FTransform targetTransform = GetViewTargetTransform(_targetIndex);
    m_runtimeCamera->SetActorTransform(targetTransform);
    m_moveStartTransform = targetTransform;

    //RuntimeCameraのFOVを設定する
    if (m_runtimeCamera->GetCameraComponent())
    {
        m_runtimeCamera->GetCameraComponent()->SetFieldOfView(m_cameraFOV);
        //最後のショットは通常操作と画角も揃え、終了直後の急なズームを防ぐ。
        if (m_viewTargets[_targetIndex] == m_playerPawn.Get())
        {
            if (UCameraComponent* camera = m_playerPawn->FindComponentByClass<UCameraComponent>())
            {
                m_runtimeCamera->GetCameraComponent()->SetFieldOfView(camera->FieldOfView);
            }
        }
    }
}

//次の通過地点への移動を開始する関数
void AIntroCutsceneDirector::StartMoveToNextTarget()
{
    //RuntimeCameraが有効でない場合はカットシーンを終了する
    if (!IsValid(m_runtimeCamera))
    {
        FinishCutscene();
        return;
    }

    //遠い地点を高速で横切らず、安全な構図同士を直接切り替える。
    if (!IsValidViewTargetIndex(m_nextTargetIndex))
    {
        m_bEndingCutscene = true;
        m_bWaitingAtTarget = false;
        m_bMovingToNextTarget = false;
        m_endHoldElapsedTime = 0.0f;
        return;
    }
    m_currentTargetIndex = m_nextTargetIndex++;
    SnapRuntimeCameraToTarget(m_currentTargetIndex);
    m_holdElapsedTime = 0.0f;
    m_bWaitingAtTarget = true;
    m_bMovingToNextTarget = false;
    if (m_playerController.IsValid() && m_playerController->PlayerCameraManager)
    {
        m_playerController->PlayerCameraManager->StopCameraFade();
        m_playerController->PlayerCameraManager->SetGameCameraCutThisFrame();
    }
}

//TravelControl位置を作成します。
FVector AIntroCutsceneDirector::BuildTravelControlLocation(const FVector& _startLocation, const FVector& _endLocation) const
{
    //ワールドを返します。
    UWorld* world = GetWorld();
    if (!world) { return (_startLocation + _endLocation) * 0.5f; }
    FCollisionQueryParams queryParams(SCENE_QUERY_STAT(IntroCameraTravelRoute), false, this);
    queryParams.AddIgnoredActor(this);
    if (m_playerPawn.IsValid())
    {
        queryParams.AddIgnoredActor(m_playerPawn.Get());
    }
    for (AActor* target : m_lookAtTargets)
    {
        if (IsValid(target))
        {
            queryParams.AddIgnoredActor(target);
        }
    }
    const auto isSegmentClear = [world, &queryParams, this](const FVector& _from, const FVector& _to)
    {
        FHitResult obstructionHit;
        return !world->SweepSingleByChannel(obstructionHit, _from, _to, FQuat::Identity, ECC_Visibility,
                                            FCollisionShape::MakeSphere(FMath::Max(35.0f, m_obstructionProbeRadius)), queryParams);
    };
    const FVector midpoint = (_startLocation + _endLocation) * 0.5f;
    FVector travelDirection = _endLocation - _startLocation;
    travelDirection.Z = 0.0f;
    travelDirection.Normalize();
    const FVector sideDirection = FVector::CrossProduct(FVector::UpVector, travelDirection).GetSafeNormal();

    const FVector candidates[] = {midpoint + FVector(0.0f, 0.0f, 420.0f), midpoint + sideDirection * 520.0f + FVector(0.0f, 0.0f, 260.0f),
                                  midpoint - sideDirection * 520.0f + FVector(0.0f, 0.0f, 260.0f),
                                  midpoint + sideDirection * 760.0f + FVector(0.0f, 0.0f, 440.0f),
                                  midpoint - sideDirection * 760.0f + FVector(0.0f, 0.0f, 440.0f)};
    for (const FVector& candidate : candidates)
    {
        if (isSegmentClear(_startLocation, candidate) && isSegmentClear(candidate, _endLocation)) { return candidate; }
    }

    //密集した森でも直線で貫通せず、最後は上空を通る安全側へ倒します。
    return midpoint + FVector(0.0f, 0.0f, 720.0f);
}

//カメラ移動区間をSweepし、壁や木を避ける経路へ補正します。
FVector AIntroCutsceneDirector::ResolveTravelCollision(const FVector& _currentLocation, const FVector& _desiredLocation) const
{
    //ワールドを返します。
    UWorld* world = GetWorld();
    if (!world) { return _desiredLocation; }
    FCollisionQueryParams queryParams(SCENE_QUERY_STAT(IntroCameraTravelCollision), false, this);
    queryParams.AddIgnoredActor(this);
    if (m_playerPawn.IsValid())
    {
        queryParams.AddIgnoredActor(m_playerPawn.Get());
    }
    for (AActor* target : m_lookAtTargets)
    {
        if (IsValid(target))
        {
            queryParams.AddIgnoredActor(target);
        }
    }
    FHitResult obstructionHit;
    //カメラ半径を保持します。
    const float cameraRadius = FMath::Max(35.0f, m_obstructionProbeRadius);
    if (!world->SweepSingleByChannel(obstructionHit, _currentLocation, _desiredLocation, FQuat::Identity, ECC_Visibility,
                                     FCollisionShape::MakeSphere(cameraRadius), queryParams))
    {
        return _desiredLocation;
    }

    //衝突面の手前かつ外側へ置き、次フレームの曲線移動で自然に回り込みます。
    return obstructionHit.Location + obstructionHit.ImpactNormal * (cameraRadius + 12.0f) + FVector(0.0f, 0.0f, 35.0f);
}

//Ease In / Ease Outの補間を行う関数
float AIntroCutsceneDirector::EaseInOut(float _alpha) const
{
    //値は0.0fから1.0fの範囲にクランプする
    const float clampedAlpha = FMath::Clamp(_alpha, 0.0f, 1.0f);

    //SmoothStepを使用しない場合は、クランプされた値をそのまま返す
    if (!m_bUseSmoothEase) { return clampedAlpha; }

    //急に動き始めず、急に止まらないのでシネマチックに見える
    return clampedAlpha * clampedAlpha * (3.0f - 2.0f * clampedAlpha);
}

//カメラの更新処理を行う関数
void AIntroCutsceneDirector::UpdateCinematicCamera(float _deltaTime)
{
    //カットシーン終了時の待機処理
    if (m_bEndingCutscene)
    {
        //カットシーン終了時の待機時間を加算する
        m_endHoldElapsedTime += _deltaTime;
        if (m_endHoldElapsedTime >= m_endHoldDuration)
        {
            FinishCutscene();
        }
        return;
    }

    //カットシーン中の待機処理
    if (m_bWaitingAtTarget)
    {
        m_holdElapsedTime += _deltaTime;
        //静止画に見えない程度の短い寄りに限定し、木を横切る大きな移動を避ける。
        const float hold = FMath::Max(2.4f, m_holdDurationAtTarget);
        const float progress = EaseInOut(FMath::Clamp(m_holdElapsedTime / hold, 0.0f, 1.0f));
        const FVector start = m_moveStartTransform.GetLocation();
        const FVector forward = m_moveStartTransform.GetRotation().GetForwardVector();
        const FVector desired = start + forward * (progress * 35.0f);
        if (m_viewTargets[m_currentTargetIndex] != m_playerPawn.Get())
        {
            const FVector safe = ResolveTravelCollision(m_runtimeCamera->GetActorLocation(), desired);
            m_runtimeCamera->SetActorLocationAndRotation(safe, CalculateLookAtRotation(safe, m_currentTargetIndex));
        }
        if (m_holdElapsedTime >= hold)
        {
            m_holdElapsedTime = 0.0f;
            StartMoveToNextTarget();
        }
        return;
    }

    //カットシーン中の移動処理
    if (!m_bMovingToNextTarget) { return; }

    //移動時間を加算する
    m_moveElapsedTime += _deltaTime;

    //移動時間が0.1秒未満の場合は、0.1秒に設定する
    const float duration = FMath::Max(0.01f, m_hiddenTravelDuration);
    //透明度を保持します。
    const float alpha = EaseInOut(FMath::Max(0.0f, m_moveElapsedTime) / duration);

    //移動開始時のTransformと移動終了時のTransformを補間して、RuntimeCameraのTransformを更新する
    //開始時に確定した安全な端点同士を補間します。
    //毎フレーム障害物判定を行うと木の境界でカメラが跳ねるため、ここでは再判定しません。
    //直線移動ではなく二次Bezier曲線を使い、木や建物の間を安全に回り込みます。
    const FVector firstHalf = FMath::Lerp(m_moveStartTransform.GetLocation(), m_moveControlLocation, alpha);
    const FVector secondHalf = FMath::Lerp(m_moveControlLocation, m_moveEndTransform.GetLocation(), alpha);
    FVector desiredLocation = FMath::Lerp(firstHalf, secondHalf, alpha);
    //FoliageにはCollisionを持たない葉があるため、Sweepだけでは貫通を検出できません。
    //ショット間は上空へ抜けるアーチを加え、非衝突の植生も視界へ入れないようにします。
    desiredLocation.Z += FMath::Sin(alpha * PI) * 1100.0f;
    //位置を保持します。
    const FVector location = ResolveTravelCollision(m_runtimeCamera->GetActorLocation(), desiredLocation);

    //回転の補間は、FRotatorを使用して行う
    const FRotator startRotation = m_moveStartTransform.GetRotation().Rotator();
    //回転を返します。
    const FRotator endRotation = m_moveEndTransform.GetRotation().Rotator();
    //回転を保持します。
    FRotator rotation = FMath::Lerp(startRotation, endRotation, alpha);
    if (m_lookAtTargets.IsValidIndex(m_nextTargetIndex) && IsValid(m_lookAtTargets[m_nextTargetIndex]))
    {
        rotation = CalculateLookAtRotation(location, m_nextTargetIndex);
    }

    //RuntimeCameraのTransformを更新する
    m_runtimeCamera->SetActorLocationAndRotation(location, rotation);

    //RuntimeCameraのFOVを設定する
    if (m_moveElapsedTime >= duration)
    {
        //移動が完了した場合は位置を確定し、現在の被写体位置で向きを再計算します。
        //Spawn直後に敵が接地して位置が変わっても、古い回転へ戻さないためです。
        m_runtimeCamera->SetActorLocation(m_moveEndTransform.GetLocation());
        m_runtimeCamera->SetActorRotation(CalculateLookAtRotation(m_moveEndTransform.GetLocation(), m_nextTargetIndex));
        if (UCameraComponent* cameraComponent = m_runtimeCamera->GetCameraComponent())
        {
            //最後のプレイヤー紹介だけは逆光を補正し、装備と姿勢を読める明るさにします。
            cameraComponent->PostProcessSettings.AutoExposureBias = m_nextTargetIndex == m_viewTargets.Num() - 1 ? -0.05f : -0.85f;
        }

        //次のショットへ到着してから暗転を解除します。
        m_currentTargetIndex = m_nextTargetIndex;
        if (m_playerController.IsValid() && m_playerController->PlayerCameraManager)
        {
            m_playerController->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, m_fadeInDuration, FLinearColor::Black, false, false);
        }
        ++m_nextTargetIndex;

        //次の通過地点が有効でない場合は、カットシーン終了フラグを設定する
        if (m_nextTargetIndex >= m_viewTargets.Num())
        {
            m_bMovingToNextTarget = false;
            m_bEndingCutscene = true;
            m_endHoldElapsedTime = 0.0f;
        }
        else
        {
            m_bMovingToNextTarget = false;
            m_bWaitingAtTarget = true;
            m_holdElapsedTime = 0.0f;
        }
    }
}

//カットシーンを終了する関数
void AIntroCutsceneDirector::SetCutsceneCrosshairSuppressed(bool _bSuppressed)
{
    //DirectorがPawnより先にBeginPlayした場合は、現在のControllerから取り直します。
    if (!m_playerPawn.IsValid() && m_playerController.IsValid())
    {
        m_playerPawn = m_playerController->GetPawn();
    }
    if (APlayerChara* player = Cast<APlayerChara>(m_playerPawn.Get()))
    {
        player->SetCrosshairSuppressed(_bSuppressed);
    }

    //BPや初期化順によって後から生成されたクロスヘアも確実に対象へ含めます。
    TArray<UUserWidget*> crosshairWidgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, crosshairWidgets, UCombatCrosshairWidget::StaticClass(), false);
    for (UUserWidget* widget : crosshairWidgets)
    {
        if (IsValid(widget))
        {
            //表示状態を設定します。
            widget->SetVisibility(_bSuppressed ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        }
    }
}

//ゲームプレイHUDHiddenをゲーム内の対象へ反映します。
void AIntroCutsceneDirector::SetGameplayHUDHidden()
{
    SetCutsceneCrosshairSuppressed(true);
    TArray<UUserWidget*> gameplayWidgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, gameplayWidgets, UUserWidget::StaticClass(), false);
    for (UUserWidget* widget : gameplayWidgets)
    {
        if (IsValid(widget))
        {
            widget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

//ゲームプレイHUDの段階表示を開始するための状態を設定します。
void AIntroCutsceneDirector::BeginGameplayHUDReveal()
{
    m_hudRevealEntries.Reset();

    //ミッション告知はEnemyCount自身が表示するため、その他のHUDだけを順番に準備する
    TArray<UUserWidget*> gameplayWidgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, gameplayWidgets, UUserWidget::StaticClass(), false);
    gameplayWidgets.Sort([](const UUserWidget& lhs, const UUserWidget& rhs) { return lhs.GetName() < rhs.GetName(); });
    int32 revealIndex = 0;
    for (UUserWidget* widget : gameplayWidgets)
    {
        if (!IsValid(widget) || widget->IsA<UEnemyCount>())
        {
            continue;
        }
        //照準は操作再開と同時に中央へ表示し、移動やフェードの対象には含めない。
        if (widget->IsA<UCombatCrosshairWidget>())
        {
            widget->SetRenderOpacity(1.0f);
            continue;
        }
        FIntroHUDRevealEntry& entry = m_hudRevealEntries.AddDefaulted_GetRef();
        entry.m_widget = widget;
        entry.m_delay = 0.85f + static_cast<float>(revealIndex) * 0.12f;
        entry.m_elapsed = 0.0f;

        widget->SetRenderOpacity(0.0f);
        widget->SetVisibility(ESlateVisibility::Collapsed);
        ++revealIndex;
    }

    //照準だけは即座に戻し、残りのHUDは設定した待機時間の後でフェード表示する。
    SetCutsceneCrosshairSuppressed(false);
    m_bHUDRevealPlaying = m_hudRevealEntries.Num() > 0;
}

//ゲームプレイHUDの段階表示を最新のゲーム状態へ更新します。
void AIntroCutsceneDirector::UpdateGameplayHUDReveal(float _deltaTime)
{
    if (!m_bHUDRevealPlaying) { return; }

    constexpr float revealDuration = 0.42f;
    //HUD内のいずれかの項目が表示Animation中かを示します。
    bool bAnyEntryAnimating = false;
    for (FIntroHUDRevealEntry& entry : m_hudRevealEntries)
    {
        UUserWidget* widget = entry.m_widget.Get();
        if (!IsValid(widget))
        {
            continue;
        }

        entry.m_elapsed += _deltaTime;
        if (entry.m_elapsed < entry.m_delay)
        {
            bAnyEntryAnimating = true;
            continue;
        }

        widget->SetVisibility(ESlateVisibility::HitTestInvisible);
        //透明度を保持します。
        const float alpha = FMath::Clamp((entry.m_elapsed - entry.m_delay) / revealDuration, 0.0f, 1.0f);
        //視点の動きが急に変わらないよう加速率を整えます。
        const float easedAlpha = 1.0f - FMath::Pow(1.0f - alpha, 3.0f);

        widget->SetRenderOpacity(easedAlpha);
        if (alpha < 1.0f)
        {
            bAnyEntryAnimating = true;
        }
    }

    m_bHUDRevealPlaying = bAnyEntryAnimating;
}

//カットシーンを完了します。
void AIntroCutsceneDirector::FinishCutscene()
{
    //カットシーンの状態をリセットする
    m_bCutscenePlaying = false;
    m_bMovingToNextTarget = false;
    m_bWaitingAtTarget = false;
    m_bEndingCutscene = false;
    if (m_playerController.IsValid() && m_playerController->PlayerCameraManager)
    {
        m_playerController->PlayerCameraManager->StopCameraFade();
    }

    //RuntimeCameraを非表示にする
    if (m_playerController.IsValid() && m_playerPawn.IsValid())
    {
        m_playerController->SetViewTargetWithBlend(m_playerPawn.Get(), m_returnBlendTime);
    }

    //プレイヤーの操作を有効化する
    if (m_bDisablePlayerControlDuringCutscene)
    {
        SetPlayerControlEnabled(true);
    }

    //カットシーン中に生成して止めていた敵を、ここで通常状態に戻します。
    for (ASpawnEnemies* spawnActor : m_spawnActorsToActivate)
    {
        if (IsValid(spawnActor))
        {
            spawnActor->ReleasePreparedEnemies();
        }
    }

    //ミッション告知の直後から、残りのHUDを時間差でスライド表示する
    BeginGameplayHUDReveal();

    //RuntimeCameraを非表示にする
    if (IsValid(m_runtimeCamera))
    {
        m_runtimeCamera->SetActorHiddenInGame(true);
        //プレイヤーカメラへの補間に必要な時間だけ残し、補間完了後に回収する。
        m_runtimeCamera->SetLifeSpan(FMath::Max(0.01f, m_returnBlendTime + 0.1f));
    }
    if (m_cinematicFillLight)
    {
        m_cinematicFillLight->SetVisibility(false);
    }

}

//レベル内でDirectorだけが破棄された場合も、視点を戻して専用カメラを回収する。
void AIntroCutsceneDirector::EndPlay(const EEndPlayReason::Type _reason)
{
    if (_reason == EEndPlayReason::Destroyed && m_playerController.IsValid() && m_bCutscenePlaying)
    {
        if (m_playerPawn.IsValid()) { m_playerController->SetViewTarget(m_playerPawn.Get()); }
        if (m_playerController->PlayerCameraManager) { m_playerController->PlayerCameraManager->StopCameraFade(); }
        if (m_bDisablePlayerControlDuringCutscene) { SetPlayerControlEnabled(true); }
        SetCutsceneCrosshairSuppressed(false);
    }
    if (IsValid(m_runtimeCamera)) { m_runtimeCamera->Destroy(); }
    m_runtimeCamera = nullptr;
    m_hudRevealEntries.Empty();
    Super::EndPlay(_reason);
}

//プレイヤーの操作を有効化または無効化する関数
void AIntroCutsceneDirector::SetPlayerControlEnabled(bool _bEnabled)
{
    //プレイヤーコントローラーが有効でない場合は処理を終了する
    if (!m_playerController.IsValid()) { return; }

    //プレイヤーのポーンを取得する
    APawn* playerPawn = m_playerPawn.Get();
    if (!IsValid(playerPawn))
    {
        playerPawn = m_playerController->GetPawn();
        m_playerPawn = playerPawn;
    }

    //IgnoreInputは呼び出し回数を内部で数えるため、終了時は必ず完全にリセットします。
    if (_bEnabled)
    {
        m_playerController->ResetIgnoreMoveInput();
        m_playerController->ResetIgnoreLookInput();

        //入力モードを保持します。
        FInputModeGameOnly inputMode;
        m_playerController->SetInputMode(inputMode);
        m_playerController->SetShowMouseCursor(false);
    }
    else
    {
        m_playerController->SetIgnoreMoveInput(true);
        m_playerController->SetIgnoreLookInput(true);
    }

    //プレイヤーのポーンが有効でない場合は処理を終了する
    if (!IsValid(playerPawn)) { return; }

    //プレイヤーのポーンの入力を有効化または無効化する
    if (_bEnabled)
    {
        playerPawn->EnableInput(m_playerController.Get());
        //続けて「UCharacterMovementComponent* movement = playerCharacter->GetCharacter…」を判定します。
        if (ACharacter* playerCharacter = Cast<ACharacter>(playerPawn))
        {
            if (UCharacterMovementComponent* movement = playerCharacter->GetCharacterMovement(); movement && movement->MovementMode == MOVE_None)
            {
                movement->SetMovementMode(MOVE_Walking);
            }
        }
    }
    else
    {
        //プレイヤーのポーンの入力を無効化する
        playerPawn->DisableInput(m_playerController.Get());

        //プレイヤーのポーンがACharacterの場合は、移動を即座に停止する
        if (ACharacter* playerCharacter = Cast<ACharacter>(playerPawn))
        {
            if (UCharacterMovementComponent* movement = playerCharacter->GetCharacterMovement())
            {
                movement->StopMovementImmediately();
            }
        }
    }

}
