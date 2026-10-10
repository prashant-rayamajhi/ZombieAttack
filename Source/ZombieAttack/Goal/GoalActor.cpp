
#include "GoalActor.h"

#include "ZombieAttack/Enemy/SpawnEnemies.h"
#include "ZombieAttack/Player/PlayerChara.h"
#include "Blueprint/UserWidget.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

//コンストラクタ
AGoalActor::AGoalActor()
    : m_pMagicCircleSystem(nullptr), m_magicCircleColorParameter(TEXT("User.Color")), m_inactiveGoalColor(FLinearColor(1.0f, 0.02f, 0.02f, 1.0f)),
      m_activeGoalColor(FLinearColor(0.02f, 1.0f, 0.15f, 1.0f)), m_goalLightIntensity(1200.0f), m_clearDelay(1.5f),
      m_gameClearLevelName(TEXT("GameClear")), m_pDynamicGoalMaterial(nullptr), m_bActivated(false), m_bTransitionRequested(false), m_visualTime(0.0f)
{
    PrimaryActorTick.bCanEverTick = true;

    //実行ファイルにも素材を含め、エディタだけで質感が表示される状態を防ぐ。
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> beaconMaterial(TEXT("/Game/Materials/Goal/M_EvacBeacon.M_EvacBeacon"));
    m_beaconMaterial = beaconMaterial.Object;

    //SphereComponentをルートコンポーネントとして作成
    m_pSphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    SetRootComponent(m_pSphereComp);

    //SphereComponentの半径を200に設定し、衝突を無効化
    m_pSphereComp->InitSphereRadius(200.f);
    m_pSphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    m_pSphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    m_pSphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    m_pSphereComp->SetGenerateOverlapEvents(true);

    //StaticMeshComponentを作成し、SphereComponentにアタッチ
    m_pMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    m_pMeshComp->SetupAttachment(m_pSphereComp);
    m_pMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    //NiagaraComponentを作成し、SphereComponentにアタッチ
    m_pMagicCircleComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("MagicCircle"));
    m_pMagicCircleComp->SetupAttachment(m_pSphereComp);
    m_pMagicCircleComp->SetAutoActivate(true);
    m_pMagicCircleComp->SetRelativeLocation(FVector::ZeroVector);

    //NiagaraのUser.Colorが効いていない場合でも、最低限赤/緑の目印が見えるようにライトも使います。
    m_pGoalLightComp = CreateDefaultSubobject<UPointLightComponent>(TEXT("GoalLight"));
    m_pGoalLightComp->SetupAttachment(m_pSphereComp);
    m_pGoalLightComp->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
    m_pGoalLightComp->SetIntensity(m_goalLightIntensity);
    m_pGoalLightComp->SetAttenuationRadius(900.0f);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));

    m_pBeaconBaseComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeaconBase"));
    m_pBeaconBaseComp->SetupAttachment(m_pSphereComp);
    m_pBeaconBaseComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    m_pBeaconBaseComp->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
    m_pBeaconBaseComp->SetRelativeScale3D(FVector(2.8f, 2.8f, 0.08f));
    if (CylinderMesh.Succeeded())
    {
        m_pBeaconBaseComp->SetStaticMesh(CylinderMesh.Object);
    }

    m_pBeaconColumnComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeaconColumn"));
    m_pBeaconColumnComp->SetupAttachment(m_pSphereComp);
    m_pBeaconColumnComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    m_pBeaconColumnComp->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
    m_pBeaconColumnComp->SetRelativeScale3D(FVector(0.42f, 0.42f, 2.1f));
    if (CylinderMesh.Succeeded())
    {
        m_pBeaconColumnComp->SetStaticMesh(CylinderMesh.Object);
    }

    m_pBeaconArrowComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeaconArrow"));
    m_pBeaconArrowComp->SetupAttachment(m_pSphereComp);
    m_pBeaconArrowComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    m_pBeaconArrowComp->SetRelativeLocation(FVector(0.0f, 0.0f, 230.0f));
    m_pBeaconArrowComp->SetRelativeRotation(FRotator(180.0f, 0.0f, 0.0f));
    m_pBeaconArrowComp->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.2f));
    if (ConeMesh.Succeeded())
    {
        m_pBeaconArrowComp->SetStaticMesh(ConeMesh.Object);
    }

    m_pStatusTextComp = CreateDefaultSubobject<UTextRenderComponent>(TEXT("GoalStatusText"));
    m_pStatusTextComp->SetupAttachment(m_pSphereComp);
    m_pStatusTextComp->SetRelativeLocation(FVector(0.0f, 0.0f, 330.0f));
    m_pStatusTextComp->SetHorizontalAlignment(EHTA_Center);
    m_pStatusTextComp->SetVerticalAlignment(EVRTA_TextCenter);
    m_pStatusTextComp->SetWorldSize(42.0f);
    m_pStatusTextComp->SetTextRenderColor(FColor(220, 20, 15));
    m_pStatusTextComp->SetText(FText::FromString(TEXT("EVACUATION LOCKED\nELIMINATE HOSTILES")));

    //魔法の柱ではなく、森林の封鎖区域を抜ける金属製の出口として組み立てる。
    static ConstructorHelpers::FObjectFinder<UStaticMesh> frameMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> steel(
        TEXT("/Game/ModularBuildingSet/materials/Metal/metal_trim_green.metal_trim_green"));
    for (int32 index = 0; index < 3; ++index)
    {
        const FName name(*FString::Printf(TEXT("ExitFrame%d"), index));
        UStaticMeshComponent* frame = CreateDefaultSubobject<UStaticMeshComponent>(name);
        frame->SetupAttachment(m_pSphereComp);
        frame->SetStaticMesh(frameMesh.Object);
        frame->SetMaterial(0, steel.Object);
        frame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        frame->SetRelativeLocation(index == 2 ? FVector(0, 0, 300) : FVector(0, index == 0 ? -170 : 170, 150));
        frame->SetRelativeScale3D(index == 2 ? FVector(0.3f, 3.7f, 0.4f) : FVector(0.3f, 0.3f, 3));
    }
}

//毎フレームの更新を行います。
void AGoalActor::Tick(float _deltaTime)
{
    //フレームごとの経過時間を使って、移動や表示の変化を更新します。
    Super::Tick(_deltaTime);
    m_visualTime += _deltaTime;
    if (m_pGoalLightComp)
    {
        const float Pulse = 0.78f + 0.22f * FMath::Sin(m_visualTime * (m_bActivated ? 5.0f : 2.0f));
        m_pGoalLightComp->SetIntensity(m_goalLightIntensity * Pulse);
    }
    if (m_pStatusTextComp)
    {
        if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
        {
            const FRotator Facing = (PlayerPawn->GetActorLocation() - m_pStatusTextComp->GetComponentLocation()).Rotation();
            m_pStatusTextComp->SetWorldRotation(FRotator(0.0f, Facing.Yaw, 0.0f));
        }
    }
}

//コンストラクタ
void AGoalActor::OnConstruction(const FTransform& _transform)
{
    Super::OnConstruction(_transform);
    ApplyGoalVisualState(m_bActivated);
}

//コンポーネントの初期化後に呼ばれる関数
void AGoalActor::PostInitializeComponents()
{
    //コンポーネントの初期化
    Super::PostInitializeComponents();

    //SphereComponentのオーバーラップイベントを設定
    if (!ResolveComponents()) { return; }

    //既存のバインドを解除してから、新しいバインドを追加
    m_pSphereComp->OnComponentBeginOverlap.RemoveDynamic(this, &AGoalActor::OnOverlapBegin);
    m_pSphereComp->OnComponentBeginOverlap.AddDynamic(this, &AGoalActor::OnOverlapBegin);
}

//コンポーネントの解決
bool AGoalActor::ResolveComponents()
{
    //ルートコンポーネントがSphereComponentでない場合、FindComponentByClassで探す
    if (!IsValid(m_pSphereComp))
    {
        m_pSphereComp = Cast<USphereComponent>(GetRootComponent());
    }
    if (!IsValid(m_pSphereComp))
    {
        m_pSphereComp = FindComponentByClass<USphereComponent>();
    }

    if (!IsValid(m_pSphereComp))
    {
        //呼び出し元へ失敗を返し、この関数でこれ以上の処理を行わないようにします。
        return false;
    }

    //メッシュコンポーネント、Niagaraコンポーネント、ライトコンポーネントを解決
    if (!IsValid(m_pMeshComp))
    {
        m_pMeshComp = FindComponentByClass<UStaticMeshComponent>();
    }
    if (!IsValid(m_pMagicCircleComp))
    {
        m_pMagicCircleComp = FindComponentByClass<UNiagaraComponent>();
    }
    if (!IsValid(m_pGoalLightComp))
    {
        m_pGoalLightComp = FindComponentByClass<UPointLightComponent>();
    }

    //呼び出し元へ成功を返し、この関数でこれ以上の処理を行わないようにします。
    return true;
}

//ゲーム開始時に呼ばれる関数
void AGoalActor::BeginPlay()
{
    //親クラスのBeginPlayを呼び出す
    Super::BeginPlay();

    //コンポーネントの解決に失敗した場合、衝突を無効化して終了
    if (!ResolveComponents())
    {
        SetActorEnableCollision(false);
        return;
    }

    //ゲーム開始時は非アクティブ状態に設定
    SetActorHiddenInGame(false);
    //旧ビーコンは参照互換のため残すが、出口の通路を隠す回転柱と矢印は表示しない。
    m_pBeaconColumnComp->SetVisibility(false);
    m_pBeaconArrowComp->SetVisibility(false);
    m_pStatusTextComp->SetRelativeLocation(FVector(0, 0, 345));
    m_pStatusTextComp->SetWorldSize(28.0f);
    SetActorEnableCollision(true);
    m_pSphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ApplyGoalVisualState(false);
}

//ゴールのビジュアル状態を適用する関数
void AGoalActor::ApplyGoalVisualState(bool _bActive)
{
    //コンポーネントの解決に失敗した場合、処理を中断
    if (!ResolveComponents()) { return; }

    //アクティブ状態に応じてゴールの色を決定
    const FLinearColor goalColor = _bActive ? m_activeGoalColor : m_inactiveGoalColor;

    //白い基本形状のままにならないよう、三つの部品へ金属素材と控えめな誘導色を設定する。
    if (m_beaconMaterial)
    {
        for (UStaticMeshComponent* part : {m_pBeaconBaseComp.Get(), m_pBeaconColumnComp.Get(), m_pBeaconArrowComp.Get()})
        {
            if (!part) { continue; }
            UMaterialInstanceDynamic* material = Cast<UMaterialInstanceDynamic>(part->GetMaterial(0));
            if (!material || material->Parent != m_beaconMaterial)
            {
                material = part->CreateDynamicMaterialInstance(0, m_beaconMaterial);
            }
            if (material) { material->SetVectorParameterValue(TEXT("BeaconColor"), goalColor * 0.025f); }
        }
    }

    //アクティブ状態に応じてゴールの表示を切り替え
    SetActorHiddenInGame(false);

    //森林の出口に魔法陣を重ねず、状態は看板と照明で伝える。
    if (m_pMagicCircleComp)
    {
        m_pMagicCircleComp->Deactivate();
        m_pMagicCircleComp->SetHiddenInGame(true);
    }

    //ライトコンポーネントの表示と色を設定
    if (m_pGoalLightComp)
    {
        m_pGoalLightComp->SetHiddenInGame(false);
        m_pGoalLightComp->SetVisibility(true);
        m_pGoalLightComp->SetLightColor(goalColor);
        m_pGoalLightComp->SetIntensity(m_goalLightIntensity);
    }

    //メッシュコンポーネントの表示とマテリアルの色を設定
    //旧Blueprintの木メッシュ参照を読み込めるようコンポーネントを残します。
    //互換性だけを維持して描画は行わず、上部の工業用ビーコンをゴールとして表示します。
    if (m_pMeshComp)
    {
        m_pMeshComp->SetVisibility(false, true);
        m_pMeshComp->SetHiddenInGame(true, true);
    }
    if (m_pStatusTextComp)
    {
        m_pStatusTextComp->SetText(_bActive ? FText::FromString(TEXT("EXIT\nPROCEED TO EXTRACTION"))
                                            : FText::FromString(TEXT("EXIT CLOSED\nCLEAR THE FOREST")));
        m_pStatusTextComp->SetTextRenderColor(_bActive ? FColor(160, 210, 166) : FColor(213, 177, 111));
    }
}

//メッシュマテリアルに色を適用する関数
void AGoalActor::ApplyColorToMeshMaterial(const FLinearColor& _goalColor)
{
    //コンポーネントの解決に失敗した場合、処理を中断
    if (!m_pMeshComp) { return; }

    //マテリアルインスタンスがまだ作成されていない場合、動的マテリアルインスタンスを作成
    if (!m_pDynamicGoalMaterial)
    {
        m_pDynamicGoalMaterial = m_pMeshComp->CreateDynamicMaterialInstance(0);
    }

    //動的マテリアルインスタンスが作成されていない場合、処理を中断
    if (!m_pDynamicGoalMaterial) { return; }

    //マテリアルのパラメータに色を設定
    m_pDynamicGoalMaterial->SetVectorParameterValue(TEXT("Color"), _goalColor);
    m_pDynamicGoalMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), _goalColor);
    m_pDynamicGoalMaterial->SetVectorParameterValue(TEXT("GoalColor"), _goalColor);
}

//ゴールをアクティブ化する関数
void AGoalActor::ActivateGoal()
{
    //すでにアクティブ化されている場合、処理を中断
    if (m_bActivated) { return; }

    //コンポーネントの解決に失敗した場合、エラーログを出力して処理を中断
    if (!ResolveComponents())
    {
        return;
    }

    //ゴールをアクティブ化し、衝突を有効化
    m_bActivated = true;
    m_bTransitionRequested = false;

    //ゴールを表示し、ビジュアル状態をアクティブに設定
    SetActorHiddenInGame(false);
    ApplyGoalVisualState(true);

    //衝突を有効化して、プレイヤーがゴールに到達できるようにする
    m_pSphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BP_OnGoalActivated();
    //ゴールActivatedを関係するオブジェクトへ通知します。
    ASpawnEnemies::NotifyGoalActivated(GetWorld());

    //ゴール到達時のUIウィジェットを表示
    if (m_goalWidgetClass)
    {
        if (UUserWidget* goalWidget = CreateWidget<UUserWidget>(GetWorld(), m_goalWidgetClass))
        {
            goalWidget->AddToViewport();
        }
    }
}

//ゴールのオーバーラップ開始時に呼ばれる関数
void AGoalActor::OnOverlapBegin(UPrimitiveComponent* _overlappedComponent, AActor* _otherActor, UPrimitiveComponent* _otherComponent,
                                int32 _otherBodyIndex, bool _bFromSweep, const FHitResult& _sweepResult)
{
    //オーバーラップイベントの条件をチェック
    if (!m_bActivated || m_bTransitionRequested || !IsValid(_otherActor) || !_otherActor->IsA<APlayerChara>()) { return; }
    //死亡直後の接触をクリアとして扱わず、ゲームオーバーとの二重遷移を防ぐ。
    APlayerChara* player = CastChecked<APlayerChara>(_otherActor);
    if (player->IsDead()) { return; }

    //コンポーネントの解決に失敗した場合、処理を中断
    if (!ResolveComponents()) { return; }

    //ゴール到達がリクエストされたことを記録し、衝突を無効化
    m_bTransitionRequested = true;
    m_pSphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BP_OnGoalReached(CastChecked<APlayerChara>(_otherActor));

    //ゴール到達時の処理をスケジュール
    TWeakObjectPtr<AGoalActor> weakThis(this);
    TWeakObjectPtr<APlayerChara> weakPlayer(player);
    GetWorldTimerManager().SetTimer(m_clearTimer,
                                    FTimerDelegate::CreateLambda(
                                        [weakThis, weakPlayer]()
                                        {
                                            if (!weakThis.IsValid()) { return; }
                                            //到達後の待ち時間に死亡した場合も、死亡画面をクリア画面で上書きしない。
                                            if (!weakPlayer.IsValid() || weakPlayer->IsDead()) { return; }
                                            AGoalActor* goalActor = weakThis.Get();
                                            if (!goalActor->m_gameClearLevelName.IsNone())
                                            {
                                                //Levelへ安全に遷移します。
                                                UGameplayStatics::OpenLevel(goalActor, goalActor->m_gameClearLevelName);
                                            }
                                        }),
                                    //ゼロ秒のTimerは予約を解除するため、即時設定でも次の更新で遷移できる長さにする。
                                    FMath::Max(0.01f, m_clearDelay), false);
}
