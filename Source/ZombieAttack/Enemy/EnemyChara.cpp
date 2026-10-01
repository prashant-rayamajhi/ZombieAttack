#include "EnemyChara.h"
#include "Components/EnemyAttackTraceComponent.h"
#include "../AIController/EnemyAIController.h"
#include "../PickUp/PickUpBase.h"
#include "../Player/PlayerChara.h"
#include "../UI/EnemyUI/EnemyHP.h"
#include "../Components/CombatFeedback/EnemyHitFeedbackComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"
#include "Components/BoxComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/DamageEvents.h"
#include "Engine/SkeletalMesh.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/UnrealType.h"
#include "ZombieAttack/Animation/Enemy/EnemyAnimationTiming.h"
#include "TimerManager.h"

//コンストラクタ
AEnemyChara::AEnemyChara()
    : m_pHealthComp(nullptr), m_pDeathMontage(nullptr), m_pAttackMontage(nullptr), m_regularDropProbability(0.22f), m_pPatrolSound(nullptr),
      m_pAttackSound(nullptr), m_pAlertSound(nullptr), m_pDeathSound(nullptr), m_pSoundAttenuation(nullptr), m_pSoundConcurrency(nullptr),
      m_patrolSoundIntervalMin(8.f), m_patrolSoundIntervalMax(20.f), m_patrolSoundHearingRange(1800.f), m_patrolWalkSpeed(120.f),
      m_chaseRunSpeed(500.f), m_bEnableVisibilityAssist(true), m_notSeenAlertTime(30.f), m_boostedNotSeenAlertTime(30.f),
      m_visibilityAssistMaxDistance(9000.f), m_visibilityAssistFocusDot(0.9f), m_visibilityAssistCheckInterval(0.2f), m_outlineReleaseDelay(0.6f),
      m_attackRange(150.f), m_attackInterval(1.5f), m_hitDamage(10), m_pAttackCollision(nullptr),
      m_pAttackWindupVFX(nullptr), m_pAttackImpactVFX(nullptr), m_pActiveAttackVFX(nullptr), m_pHitFeedbackComponent(nullptr),
      m_pPlayerChara(nullptr), m_moveState(EEnemyMoveState::Patrol), m_bIsDead(false), m_bIsAttacking(false), m_bHitAppliedThisAttack(false),
      m_bRedOutlineEnabled(false), m_bDefeatBroadcast(false), m_bSearchAssistBoosted(false), m_attackCooldownRemaining(0.f), m_notSeenTimer(0.f),
      m_visibilityAssistCheckAccumulator(0.f), m_outlineReleaseAccumulator(0.f), m_bAttackHitThisSwing(false)
{
    SetLocomotionAssets(TEXT("/Game/Assets/Enemy/Animation/Minion/AnimationSequence"), TEXT("Zombie_Idle"),
                        TEXT("Walking__1_"), TEXT("Zombie_Running"), TEXT("Zombie_Scream"));
    //通常敵の攻撃VFXは使用せず、アニメーションと被弾画面演出で伝えます。
    m_pAttackWindupVFX = nullptr;
    m_pAttackImpactVFX = nullptr;
    //Tickを有効化する
    PrimaryActorTick.bCanEverTick = true;
    //手足が高速に動く攻撃でも、骨の更新後に連続した接触範囲を調べる。
    CreateDefaultSubobject<UEnemyAttackTraceComponent>(TEXT("AttackTrace"));
    AIControllerClass = AEnemyAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    //ウィジェットコンポーネントを作成し、メッシュにアタッチする
    m_pHealthComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
    m_pHealthComp->SetupAttachment(GetMesh());
    m_pHealthComp->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
    m_pHealthComp->SetWidgetSpace(EWidgetSpace::Screen);
    m_pHealthComp->SetDrawSize(FVector2D(150.f, 24.f));
    m_pHealthComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    //命中表現を敵本体から分離し、全敵クラスで同じ処理を共有する
    m_pHitFeedbackComponent = CreateDefaultSubobject<UEnemyHitFeedbackComponent>(TEXT("HitFeedbackComponent"));

    //攻撃判定用のボックスコンポーネントを作成し、右手のソケットにアタッチする
    m_pAttackCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("AttackCollision"));
    if (m_pAttackCollision)
    {
        m_pAttackCollision->SetupAttachment(GetMesh(), TEXT("hand_r"));
        m_pAttackCollision->SetBoxExtent(FVector(18.0f, 18.0f, 28.0f));
        m_pAttackCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        m_pAttackCollision->SetCollisionObjectType(ECC_WorldDynamic);
        m_pAttackCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
        m_pAttackCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        m_pAttackCollision->SetGenerateOverlapEvents(false);
    }

    //RVO回避を有効化する
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->bUseRVOAvoidance = true;
        Movement->AvoidanceConsiderationRadius = 500.f;
        Movement->AvoidanceWeight = 0.55f;
    }

    //カプセルコンポーネントがナビゲーションに影響を与えないようにする
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCanEverAffectNavigation(false);
    }
}

//BulletImpactFeedbackを再生します。
void AEnemyChara::PlayBulletImpactFeedback(const FHitResult& _hitResult, const FVector& _shotDirection)
{
    if (!m_pHitFeedbackComponent || m_bIsDead) { return; }

    m_pHitFeedbackComponent->PlayBulletImpact(_hitResult, _shotDirection);
}

//ゲーム開始時に呼ばれる
void AEnemyChara::BeginPlay()
{
    //親クラスのBeginPlayを呼び出す
    Super::BeginPlay();
    InitializeEnemyAnimation();

    //配置済みBPの設定漏れや別Skeletonの混入を開始時に検出します。

    //BPに古い初期値が保存されていても、30秒経過後に統一します。
    m_bEnableVisibilityAssist = true;
    m_notSeenAlertTime = 30.0f;
    m_boostedNotSeenAlertTime = 30.0f;
    m_visibilityAssistMaxDistance = 100000.0f;
    m_pAttackWindupVFX = nullptr;
    m_pAttackImpactVFX = nullptr;

    //体力を最大値に設定する
    m_hp = m_maxHp;
    m_pPlayerChara = Cast<APlayerChara>(UGameplayStatics::GetPlayerCharacter(this, 0));

    //体力ウィジェットを取得し、所有者を設定する
    if (UEnemyHP* HealthWidget = Cast<UEnemyHP>(m_pHealthComp->GetUserWidgetObject()))
    {
        HealthWidget->SetOwner(this);
        m_onHealthChanged.AddDynamic(HealthWidget, &UEnemyHP::UpdateHealthUI);
        HealthWidget->UpdateHealthUI();
    }
    m_pHealthComp->SetHiddenInGame(true);

    //攻撃判定のオーバーラップイベントをバインドし、攻撃判定を無効化する
    if (m_pAttackCollision)
    {
        m_pAttackCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnemyChara::OnAttackCollisionOverlap);
        PrepareAttackContact(TEXT("RightHand"));
        SetAttackCollisionEnabled(false);
    }

    //キャラクターの回転設定を変更する
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->bUseControllerDesiredRotation = false;
        Movement->bOrientRotationToMovement = true;
        Movement->RotationRate = FRotator(0.f, 220.f, 0.f);
        Movement->MaxWalkSpeed = m_patrolWalkSpeed;
    }

    //パトロールサウンドの再生タイマーを設定する
    if (m_pPatrolSound)
    {
        GetWorldTimerManager().SetTimer(m_patrolSoundTimer, this, &AEnemyChara::PlayPatrolSoundAndReschedule,
                                        FMath::FRandRange(m_patrolSoundIntervalMin, FMath::Max(m_patrolSoundIntervalMin, m_patrolSoundIntervalMax)),
                                        false);
    }
}

//毎フレーム呼ばれる
void AEnemyChara::Tick(float _deltaTime)
{
    //親クラスのTickを呼び出す
    Super::Tick(_deltaTime);
    if (m_bIsDead) { return; }

    //ボスも通常敵も、攻撃と咆哮で停止している間は追跡速度へ戻さない
    if (AEnemyAIController* controller = Cast<AEnemyAIController>(GetController()))
    {
        if (!m_bIsAttacking && !controller->IsAlertReactionActive())
        {
            const bool chasing = controller->GetCurrentState() == EEnemyAIState::Chase;
            GetCharacterMovement()->MaxWalkSpeed = chasing ? m_chaseRunSpeed : m_patrolWalkSpeed;
        }
    }

    //AI移動でカプセルが動いた場合も、アニメーション側へ実速度を毎フレーム渡す
    SynchronizeLocomotionAnimation();

    //攻撃クールダウンを減算する
    m_attackCooldownRemaining = FMath::Max(0.f, m_attackCooldownRemaining - _deltaTime);

    //攻撃ロジックを処理するかどうかを確認し、必要に応じて処理する
    if (ShouldUseHandleAttackLogic())
    {
        HandleAttackLogic(_deltaTime);
    }

    //アラートアウトラインの更新を行う
    UpdateAlertOutline(_deltaTime);
}

//実速度をAnimInstanceへ渡し、敵の足滑りを抑えます。
void AEnemyChara::SynchronizeLocomotionAnimation()
{
    //メッシュを返します。
    USkeletalMeshComponent* meshComponent = GetMesh();
    UAnimInstance* animInstance = meshComponent ? meshComponent->GetAnimInstance() : nullptr;
    if (!animInstance) { return; }

    //速度を返します。
    float groundSpeed = GetVelocity().Size2D();

    //停止時の微小な移動値や、攻撃中の押し出しで歩行アニメーションを再生させない
    if (groundSpeed < 4.0f || m_bIsAttacking || m_bIsDead)
    {
        groundSpeed = 0.0f;
    }
    if (FFloatProperty* speedProperty = FindFProperty<FFloatProperty>(animInstance->GetClass(), TEXT("Speed")))
    {
        speedProperty->SetPropertyValue_InContainer(animInstance, groundSpeed);
    }
    else if (FDoubleProperty* preciseSpeed = FindFProperty<FDoubleProperty>(animInstance->GetClass(), TEXT("Speed")))
    {
        preciseSpeed->SetPropertyValue_InContainer(animInstance, groundSpeed);
    }
}

//受けたダメージを体力へ反映します。
float AEnemyChara::TakeDamage(float _damageAmount, const FDamageEvent& _damageEvent, AController* _eventInstigator, AActor* _damageCauser)
{
    //死亡している場合はダメージを受け付けない
    if (m_bIsDead) { return 0.f; }

    //親クラスのTakeDamageを呼び出し、適用されたダメージ量を取得する
    const float AppliedDamage = Super::TakeDamage(_damageAmount, _damageEvent, _eventInstigator, _damageCauser);
    if (AppliedDamage <= 0.f) { return 0.f; }

    //体力を減算し、0から最大体力の範囲にクランプする
    m_hp = FMath::Clamp(m_hp - AppliedDamage, 0.f, m_maxHp);

    //体力が変化したことを通知する
    m_onHealthChanged.Broadcast();

    //アラートアウトラインのタイマーをリセットし、体力バーを一時的に表示する
    ResetNotSeenTimer();
    ShowHealthBarTemporarily();

    //体力が0以下になった場合、死亡処理を行う
    if (m_hp <= 0.f)
    {
        if (_eventInstigator)
        {
            if (APlayerChara* defeatingPlayer = Cast<APlayerChara>(_eventInstigator->GetPawn()))
            {
                defeatingPlayer->NotifyEnemyDefeated();

                //射撃による致死ダメージだけに絞り、近接攻撃や通常命中の操作感を守ります。
                if (_damageEvent.IsOfType(FPointDamageEvent::ClassID))
                {
                    defeatingPlayer->PlayKillHitStop();
                }
            }
        }
        m_bIsDead = true;
        PlayDeathAnimationAndDie();
        return AppliedDamage;
    }

    //プレイヤーキャラクターがまだ取得されていない場合、取得する
    if (!m_pPlayerChara)
    {
        m_pPlayerChara = GetPlayerCharacter();
    }

    //AIコントローラーを取得し、現在の状態がChaseまたはAttackでない場合、アラートサウンドを再生し、追跡を開始する
    if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
    {
        //現在の状態を返します。
        if (m_pPlayerChara && AIController->GetCurrentState() != EEnemyAIState::Chase && AIController->GetCurrentState() != EEnemyAIState::Attack)
        {
            PlayEnemySound(m_pAlertSound);
            AIController->StartChase(m_pPlayerChara);
        }
    }
    return AppliedDamage;
}

//攻撃判定の有効区間に合わせて、手元へ小さな軌跡エフェクトを表示する
void AEnemyChara::BeginAttackVFXWindow()
{
    EndAttackVFXWindow();
    if (!m_pAttackWindupVFX || !m_pAttackCollision) { return; }

    m_pActiveAttackVFX =
        //SystemAttachedを作成します。
        UNiagaraFunctionLibrary::SpawnSystemAttached(m_pAttackWindupVFX, m_pAttackCollision, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
                                                     FVector(0.13f), EAttachLocation::KeepRelativeOffset, false, ENCPoolMethod::None);
}

//攻撃判定と同時に軌跡エフェクトを停止する
void AEnemyChara::EndAttackVFXWindow()
{
    if (!m_pActiveAttackVFX) { return; }
    m_pActiveAttackVFX->Deactivate();
    m_pActiveAttackVFX = nullptr;
}

//攻撃をトリガーする（クールダウンをリセットする）
void AEnemyChara::TriggerAttack() { m_attackCooldownRemaining = 0.f; }

//MontageCompatibleかを判定します。
bool AEnemyChara::IsMontageCompatible(const UAnimMontage* _montage) const
{
    //メッシュを返します。
    const USkeletalMeshComponent* mesh = GetMesh();
    const USkeletalMesh* skeletalMesh = mesh ? mesh->GetSkeletalMeshAsset() : nullptr;
    return _montage && skeletalMesh && _montage->GetSkeleton() == skeletalMesh->GetSkeleton();
}

//プレイヤーまでの有効距離を取得する（カプセルの半径を考慮する）
float AEnemyChara::GetEffectiveDistanceToPlayer() const
{
    //プレイヤーキャラクターが存在しない場合、最大値を返す
    if (!m_pPlayerChara) { return TNumericLimits<float>::Max(); }

    //プレイヤーキャラクターとの2D距離を計算し、敵とプレイヤーのカプセル半径を考慮して有効距離を返す
    const float CenterDistance = FVector::Dist2D(GetActorLocation(), m_pPlayerChara->GetActorLocation());
    //敵半径を保持します。
    const float EnemyRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
    //プレイヤー半径を保持します。
    const float PlayerRadius = m_pPlayerChara->GetCapsuleComponent()->GetScaledCapsuleRadius();
    return FMath::Max(0.f, CenterDistance - EnemyRadius - PlayerRadius);
}

//体力バーを一時的に表示する
void AEnemyChara::ShowHealthBarTemporarily(float _displaySeconds)
{
    //体力コンポーネントが存在しない場合は処理を終了する
    if (!m_pHealthComp) { return; }

    //体力バーを表示し、非表示タイマーをクリアする
    m_pHealthComp->SetHiddenInGame(false);
    GetWorldTimerManager().ClearTimer(m_healthBarHideTimer);

    //TWeakObjectPtrを使用して、ラムダ内でのthisポインタの有効性を確認する
    TWeakObjectPtr<AEnemyChara> WeakThis(this);
    GetWorldTimerManager().SetTimer(m_healthBarHideTimer,
                                    FTimerDelegate::CreateLambda(
                                        [WeakThis]()
                                        {
                                            if (WeakThis.IsValid() && !WeakThis->m_bIsDead && WeakThis->m_pHealthComp)
                                            {
                                                WeakThis->m_pHealthComp->SetHiddenInGame(true);
                                            }
                                        }),
                                    FMath::Max(0.f, _displaySeconds), false);
}

//パトロールサウンドを再生し、次の再生タイマーを設定する
void AEnemyChara::PlayPatrolSoundAndReschedule()
{
    //プレイヤーキャラクターがまだ取得されていない場合、取得する
    if (!m_bIsDead && !m_pPlayerChara)
    {
        m_pPlayerChara = GetPlayerCharacter();
    }

    //死亡していない場合、プレイヤーキャラクターが存在し、距離がパトロールサウンドの聴取範囲内であれば、パトロールサウンドを再生する
    if (!m_bIsDead && m_pPlayerChara && FVector::Dist(GetActorLocation(), m_pPlayerChara->GetActorLocation()) <= m_patrolSoundHearingRange)
    {
        PlayEnemySound(m_pPatrolSound);
    }

    //死亡していない場合、次のパトロールサウンド再生タイマーを設定する
    if (!m_bIsDead)
    {
        GetWorldTimerManager().SetTimer(m_patrolSoundTimer, this, &AEnemyChara::PlayPatrolSoundAndReschedule,
                                        FMath::FRandRange(m_patrolSoundIntervalMin, FMath::Max(m_patrolSoundIntervalMin, m_patrolSoundIntervalMax)),
                                        false);
    }
}

//指定されたサウンドを再生する（音量倍率を考慮する）
void AEnemyChara::PlayEnemySound(USoundBase* _sound, float _volumeMultiplier) const
{
    //サウンドが存在しない場合は処理を終了する
    if (!_sound) { return; }

    //音量倍率を0以上にクランプする
    float EffectiveVolume = FMath::Max(0.f, _volumeMultiplier);
    if (!m_pSoundAttenuation)
    {
        //サウンド減衰が設定されていない場合、プレイヤーとの距離に応じて音量を減衰させる
        if (const APlayerChara* Player = GetPlayerCharacter())
        {
            //距離に応じて音量を減衰させる
            const float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
            const float InnerRadius = 200.f;
            const float FalloffRange = FMath::Max(InnerRadius + 1.f, m_patrolSoundHearingRange);
            //透明度を保持します。
            const float Alpha = FMath::Clamp((Distance - InnerRadius) / (FalloffRange - InnerRadius), 0.f, 1.f);
            EffectiveVolume *= FMath::Square(1.f - Alpha);
        }
    }

    //有効音量がほとんどない場合は再生をスキップする
    if (EffectiveVolume <= KINDA_SMALL_NUMBER) { return; }

    //サウンドを指定された位置で再生する
    UGameplayStatics::PlaySoundAtLocation(this, _sound, GetActorLocation(), FRotator::ZeroRotator, EffectiveVolume, 1.f, 0.f, m_pSoundAttenuation,
                                          m_pSoundConcurrency, const_cast<AEnemyChara*>(this));
}

//プレイヤーキャラクターを取得する
APlayerChara* AEnemyChara::GetPlayerCharacter() const { return Cast<APlayerChara>(UGameplayStatics::GetPlayerCharacter(this, 0)); }

//死亡アニメーションを再生し、死亡処理を行う
void AEnemyChara::PlayDeathAnimationAndDie()
{
    //死亡直後から攻撃通知と手の当たり判定を受け付けない
    m_bIsDead = true;
    m_bIsAttacking = false;
    SetAttackCollisionEnabled(false);
    EndAttackVFXWindow();
    //死亡フラグを設定し、攻撃関連のタイマーをクリアする
    GetWorldTimerManager().ClearTimer(m_attackEndTimer);
    GetWorldTimerManager().ClearTimer(m_deathPoseFreezeTimer);
    GetWorldTimerManager().ClearTimer(m_patrolSoundTimer);
    GetWorldTimerManager().ClearTimer(m_healthBarHideTimer);

    //AIコントローラーを取得し、すべてのロジックを停止する
    if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
    {
        AIController->StopAllLogic();
    }

    //移動を無効化し、カプセルコンポーネントの衝突を無効化する
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    //体力コンポーネントを非表示にし、赤いアウトラインを無効化する
    if (m_pHealthComp)
    {
        m_pHealthComp->SetHiddenInGame(true);
    }
    SetRedOutlineColor(false);
    PlayEnemySound(m_pDeathSound);

    //死亡モンタージュを再生し、再生時間を取得する
    if (USkeletalMeshComponent* mesh = GetMesh())
    {
        mesh->bPauseAnims = false;
    }
    float deathDuration = 0.0f;
    //死亡アニメーションを保持します。
    UAnimSequenceBase* deathAnimation = nullptr;

    //死亡時はAnimBPの状態遷移を通さず、Montage内の正しいSequenceを
    //Single Nodeで再生します。これにより別敵種のDeath State混入を防ぎます。
    if (IsMontageCompatible(m_pDeathMontage) && m_pDeathMontage->SlotAnimTracks.Num() > 0 &&
        m_pDeathMontage->SlotAnimTracks[0].AnimTrack.AnimSegments.Num() > 0)
    {
        deathAnimation = m_pDeathMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].GetAnimReference();
    }

    //メッシュを返します。
    USkeletalMeshComponent* mesh = GetMesh();
    if (mesh && deathAnimation && mesh->GetSkeletalMeshAsset() && deathAnimation->GetSkeleton() == mesh->GetSkeletalMeshAsset()->GetSkeleton())
    {
        mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        mesh->PlayAnimation(deathAnimation, false);
        deathDuration = deathAnimation->GetPlayLength();

        //最終フレームの直前で停止し、Idleへ一瞬戻る現象を防ぎます。
        GetWorldTimerManager().SetTimer(m_deathPoseFreezeTimer, this, &AEnemyChara::FreezeDeathPose,
                                        FMath::Max(0.05f, deathDuration - (1.0f / 30.0f)), false);
    }

    GetWorldTimerManager().SetTimer(m_deathTimerHandle, this, &AEnemyChara::Die, deathDuration > 0.0f ? deathDuration + 0.08f : 0.1f, false);
}

//死亡Montageの最終姿勢を固定し、消える直前の立ち上がりを防ぎます。
void AEnemyChara::FreezeDeathPose()
{
    if (!m_bIsDead) { return; }
    if (USkeletalMeshComponent* mesh = GetMesh())
    {
        //死亡Montage終了後にIdleへ戻って一瞬立ち上がるのを防ぎます。
        mesh->bPauseAnims = true;
    }
}

//死亡処理を行う
void AEnemyChara::Die()
{
    //死亡イベントがまだブロードキャストされていない場合、ブロードキャストする
    if (!m_bDefeatBroadcast)
    {
        m_bDefeatBroadcast = true;
        m_onEnemyDefeated.Broadcast(this);
    }

    //アイテムをドロップする
    DropItem();

    //親クラスのDieを呼び出す
    Super::Die();
}

//アイテムをドロップする
void AEnemyChara::DropItem()
{
    //ドロップ抽選に外れた場合、または生成先Worldがない場合はアイテムを生成しません。
    if (FMath::FRand() > m_regularDropProbability || !GetWorld()) { return; }
        TSubclassOf<APickUpBase> pickupClass = m_pPickUpClass;
        if (!pickupClass)
        {
            pickupClass = LoadClass<APickUpBase>(nullptr, TEXT("/Game/Blueprints/Weapons/BP_PickUp_AR.BP_PickUp_AR_C"));
        }
        //代替クラスも取得できなかった場合は、無効なクラスでActorを生成しないよう終了します。
        if (!pickupClass) { return; }

        //今回生成するDrop Itemが弾薬かを示します。
        const bool bAmmo = FMath::FRand() <= 0.65f;
        //弾薬DropをAR用として生成するかを示します。
        const bool bUseARAmmo = bAmmo && m_pPlayerChara && m_pPlayerChara->GetCurrentSlot() == EWeaponSlot::AR;
        //アイテム種類を保持します。
        const EItemType itemType = bAmmo ? (bUseARAmmo ? EItemType::EIT_ARAmmo : EItemType::EIT_Ammo) : EItemType::EIT_Health;
        //アイテム値を保持します。
        const float itemValue = bAmmo ? static_cast<float>(FMath::RandRange(bUseARAmmo ? 18 : 8, bUseARAmmo ? 32 : 15)) : 25.0f;
        FActorSpawnParameters spawnParameters;
        spawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        const FVector dropLocation = GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
        if (APickUpBase* pickup = GetWorld()->SpawnActor<APickUpBase>(pickupClass, dropLocation, FRotator::ZeroRotator, spawnParameters))
        {
            pickup->PickUpItem(itemType, itemValue);
        }
}

//赤いアウトラインの有効化/無効化を設定する
void AEnemyChara::EnableRedOutline(bool _bEnable) { SetRedOutlineColor(_bEnable); }

//赤いアウトラインの有効化/無効化を設定する（内部処理）
void AEnemyChara::SetRedOutlineColor(bool _bEnable)
{
    m_bRedOutlineEnabled = _bEnable;
    if (USkeletalMeshComponent* enemyMesh = GetMesh())
    {
        enemyMesh->SetRenderCustomDepth(_bEnable);
        enemyMesh->SetCustomDepthStencilValue(_bEnable ? 2 : 1);
    }
}

//アラートアウトラインのタイマーをリセットする
void AEnemyChara::ResetNotSeenTimer()
{
    //アラートアウトラインのタイマーとリリースアキュムレータをリセットし、赤いアウトラインを無効化する
    m_notSeenTimer = 0.f;
    m_outlineReleaseAccumulator = 0.f;
    if (m_bRedOutlineEnabled)
    {
        SetRedOutlineColor(false);
    }
}

//検索アシストの強化状態を設定する
void AEnemyChara::SetSearchAssistBoosted(bool _bBoosted)
{
    //検索アシストの強化状態を設定し、強化されている場合はアラートアウトラインのタイマーを調整する
    m_bSearchAssistBoosted = _bBoosted;
    if (m_bSearchAssistBoosted)
    {
        m_notSeenTimer = FMath::Max(m_notSeenTimer, FMath::Max(0.f, m_boostedNotSeenAlertTime - m_visibilityAssistCheckInterval));
    }
}

//プレイヤーから明確に見えるかどうかを判定する
bool AEnemyChara::IsClearlyVisibleToPlayer() const
{
    //プレイヤーキャラクターとプレイヤーコントローラーを取得する
    const APlayerChara* Player = GetPlayerCharacter();
    //プレイヤーコントローラーを保持します。
    APlayerController* PlayerController = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
    if (!Player || !PlayerController) { return false; }

    //プレイヤーのカメラ位置と回転を取得する
    FVector CameraLocation;
    //カメラ回転を保持します。
    FRotator CameraRotation;
    PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

    //ターゲット位置を計算し、プレイヤーのカメラからターゲットまでの方向と距離を計算する
    const FVector TargetLocation = GetActorLocation() + FVector(0.f, 0.f, 80.f);
    const FVector ToEnemy = TargetLocation - CameraLocation;
    //距離を保持します。
    const float Distance = ToEnemy.Size();
    if (Distance <= 1.f) { return true; }

    //プレイヤーのカメラの視線方向とターゲットへの方向のドット積を計算し、視線がターゲットに向いているかどうかを判定する
    const FVector Direction = ToEnemy / Distance;
    if (FVector::DotProduct(CameraRotation.Vector(), Direction) < m_visibilityAssistFocusDot) { return false; }

    //ターゲット位置をスクリーン座標に変換し、スクリーン内に収まっているかどうかを判定する
    FVector2D ScreenPosition;
    if (!PlayerController->ProjectWorldLocationToScreen(TargetLocation, ScreenPosition, true)) { return false; }

    //スクリーン座標がビューポート内に収まっているかどうかを判定する
    int32 ViewportX = 0;
    int32 ViewportY = 0;
    PlayerController->GetViewportSize(ViewportX, ViewportY);
    if (ViewportX <= 0 || ViewportY <= 0 || ScreenPosition.X < 0.f || ScreenPosition.X > ViewportX || ScreenPosition.Y < 0.f ||
        ScreenPosition.Y > ViewportY)
    {
        //呼び出し元へ失敗を返し、この関数でこれ以上の処理を行わないようにします。
        return false;
    }

    //プレイヤーのカメラからターゲットまでのラインをトレースし、視界が遮られていないかどうかを判定する
    FHitResult visibilityHit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyVisibilityAssist), true, Player);
    Params.AddIgnoredActor(Player);
    //ワールドを返します。
    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(visibilityHit, CameraLocation, TargetLocation, ECC_Visibility, Params);

    //視界が遮られていない場合、またはヒットしたアクターが自分自身である場合、明確に見えると判定する
    return !bBlocked || visibilityHit.GetActor() == this;
}

//アラートアウトラインの更新処理を行う
void AEnemyChara::UpdateAlertOutline(float DeltaTime)
{
    //検索アシストが無効化されているか、死亡している場合は処理を終了する
    if (!m_bEnableVisibilityAssist || m_bIsDead) { return; }

    //アラートアウトラインのチェック間隔を累積し、指定された間隔に達していない場合は処理を終了する
    m_visibilityAssistCheckAccumulator += DeltaTime;
    if (m_visibilityAssistCheckAccumulator < m_visibilityAssistCheckInterval) { return; }

    //チェック間隔に達した場合、累積時間を取得し、累積時間をリセットする
    const float CheckDelta = m_visibilityAssistCheckAccumulator;
    m_visibilityAssistCheckAccumulator = 0.f;

    //プレイヤーキャラクターを取得し、存在しない場合は処理を終了する
    const APlayerChara* Player = GetPlayerCharacter();
    if (!Player) { return; }

    //プレイヤーとの距離を計算し、指定された最大距離を超えている場合はアラートアウトラインを無効化する
    const float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
    if (Distance > m_visibilityAssistMaxDistance)
    {
        m_notSeenTimer = 0.f;
        m_outlineReleaseAccumulator = 0.f;
        SetRedOutlineColor(false);
        return;
    }

    //プレイヤーから明確に見える場合、アラートアウトラインのタイマーをリセットし、赤いアウトラインが有効化されている場合はリリースアキュムレータを更新する
    if (IsClearlyVisibleToPlayer())
    {
        m_notSeenTimer = 0.f;
        if (m_bRedOutlineEnabled)
        {
            m_outlineReleaseAccumulator += CheckDelta;
            if (m_outlineReleaseAccumulator >= m_outlineReleaseDelay)
            {
                SetRedOutlineColor(false);
                m_outlineReleaseAccumulator = 0.f;
            }
        }
        return;
    }

    //プレイヤーから明確に見えない場合、アラートアウトラインのリリースアキュムレータをリセットし、タイマーを更新する
    m_outlineReleaseAccumulator = 0.f;
    m_notSeenTimer += CheckDelta;
    const float RequiredDelay = m_bSearchAssistBoosted ? m_boostedNotSeenAlertTime : m_notSeenAlertTime;

    //アラートアウトラインが有効化されていない場合、指定された遅延時間を超えた場合に赤いアウトラインを有効化する
    if (!m_bRedOutlineEnabled && m_notSeenTimer >= RequiredDelay)
    {
        SetRedOutlineColor(true);
    }
}
