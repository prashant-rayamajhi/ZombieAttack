
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
      m_activeGoalColor(FLinearColor(0.02f, 1.0f, 0.15f, 1.0f)), m_goalLightIntensity(7000.0f), m_clearDelay(1.5f),
      m_gameClearLevelName(TEXT("GameClear")), m_pDynamicGoalMaterial(nullptr), m_bActivated(false), m_bTransitionRequested(false), m_visualTime(0.0f)
{
    PrimaryActorTick.bCanEverTick = true;

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

    //CylinderMeshは、CylinderMeshの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    //ConeMeshは、ConeMeshの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));

    m_pBeaconBaseComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeaconBase"));
    m_pBeaconBaseComp->SetupAttachment(m_pSphereComp);
    m_pBeaconBaseComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    m_pBeaconBaseComp->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
    m_pBeaconBaseComp->SetRelativeScale3D(FVector(2.8f, 2.8f, 0.08f));
    //「CylinderMesh.Succeeded()」が成立するとき、SetStaticMeshを呼び出します。
    if (CylinderMesh.Succeeded())
    {
        m_pBeaconBaseComp->SetStaticMesh(CylinderMesh.Object);
    }

    m_pBeaconColumnComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeaconColumn"));
    m_pBeaconColumnComp->SetupAttachment(m_pSphereComp);
    m_pBeaconColumnComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    m_pBeaconColumnComp->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
    m_pBeaconColumnComp->SetRelativeScale3D(FVector(0.42f, 0.42f, 2.1f));
    //「CylinderMesh.Succeeded()」が成立するとき、SetStaticMeshを呼び出します。
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
    //「ConeMesh.Succeeded()」が成立するとき、SetStaticMeshを呼び出します。
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
}

//毎フレームの更新を行います。
void AGoalActor::Tick(float DeltaTime)
{
    //フレームごとの経過時間を使って、移動や表示の変化を更新します。
    Super::Tick(DeltaTime);
    m_visualTime += DeltaTime;

    //「m_pBeaconBaseComp」が成立するとき、AddLocalRotationを呼び出します。
    if (m_pBeaconBaseComp)
    {
        m_pBeaconBaseComp->AddLocalRotation(FRotator(0.0f, DeltaTime * (m_bActivated ? 85.0f : 22.0f), 0.0f));
    }
    //「m_pBeaconArrowComp」が成立するとき、Sinを呼び出します。
    if (m_pBeaconArrowComp)
    {
        //Hoverは、FMath::Sin(m_visualTime * 2.5f) * 18.0fから算出した数値を後続の判定または計算に使います。
        const float Hover = FMath::Sin(m_visualTime * 2.5f) * 18.0f;
        m_pBeaconArrowComp->SetRelativeLocation(FVector(0.0f, 0.0f, 230.0f + Hover));
        m_pBeaconArrowComp->AddLocalRotation(FRotator(0.0f, DeltaTime * 55.0f, 0.0f));
    }
    //「m_pBeaconColumnComp」が成立するとき、Sinを呼び出します。
    if (m_pBeaconColumnComp)
    {
        //BreathingScaleは、1.0f + FMath::Sin(m_visualTime * 3.0f) * 0.035fから算出した数値を後続の判定または計算に使います。
        const float BreathingScale = 1.0f + FMath::Sin(m_visualTime * 3.0f) * 0.035f;
        m_pBeaconColumnComp->SetRelativeScale3D(FVector(0.42f * BreathingScale, 0.42f * BreathingScale, 2.1f));
    }
    //「m_pGoalLightComp」が成立するとき、Sinを呼び出します。
    if (m_pGoalLightComp)
    {
        //Pulseは、0.78f + 0.22f * FMath::Sin(m_visualTime * (m_bActivated ? 5.0f : 2.0f))から算出した数値を後続の判定または計算に使います。
        const float Pulse = 0.78f + 0.22f * FMath::Sin(m_visualTime * (m_bActivated ? 5.0f : 2.0f));
        m_pGoalLightComp->SetIntensity(m_goalLightIntensity * Pulse);
    }
    //「m_pStatusTextComp」が成立するとき、続けて「const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0)」を判定します。
    if (m_pStatusTextComp)
    {
        //「const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0)」が成立するとき、GetActorLocationを呼び出します。
        if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
        {
            //Facingは、(PlayerPawn->GetActorLocation() - m_pStatusTextComp->GetComponentLocati…から求めた空間情報を位置または向きの計算に使います。
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

    //「!IsValid(m_pSphereComp)」が成立するとき、m_pSphereCompを更新します。
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

    //「!IsValid(m_pMagicCircleComp)」が成立するとき、m_pMagicCircleCompを更新します。
    if (!IsValid(m_pMagicCircleComp))
    {
        m_pMagicCircleComp = FindComponentByClass<UNiagaraComponent>();
    }

    //「!IsValid(m_pGoalLightComp)」が成立するとき、m_pGoalLightCompを更新します。
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

    //アクティブ状態に応じてゴールの表示を切り替え
    SetActorHiddenInGame(false);

    //Niagaraコンポーネントの表示と色を設定
    if (m_pMagicCircleComp)
    {
        m_pMagicCircleComp->SetHiddenInGame(false);
        //「m_pMagicCircleSystem」が成立するとき、SetAssetを呼び出します。
        if (m_pMagicCircleSystem)
        {
            m_pMagicCircleComp->SetAsset(m_pMagicCircleSystem);
        }

        //「!m_pMagicCircleComp->IsActive()」が成立するとき、Activateを呼び出します。
        if (!m_pMagicCircleComp->IsActive())
        {
            m_pMagicCircleComp->Activate(true);
        }

        //Niagara側のUser Parameter名が違っても確認しやすいように、よく使う名前にも送る
        m_pMagicCircleComp->SetVariableLinearColor(m_magicCircleColorParameter, goalColor);
        m_pMagicCircleComp->SetVariableLinearColor(TEXT("User.Color"), goalColor);
        m_pMagicCircleComp->SetVariableLinearColor(TEXT("User.MagicColor"), goalColor);
        m_pMagicCircleComp->SetVariableLinearColor(TEXT("User.GoalColor"), goalColor);
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

    //「m_pStatusTextComp」が成立するとき、SetTextを呼び出します。
    if (m_pStatusTextComp)
    {
        m_pStatusTextComp->SetText(_bActive ? FText::FromString(TEXT("EVACUATION READY\nENTER THE GREEN BEACON"))
                                            : FText::FromString(TEXT("EVACUATION LOCKED\nELIMINATE HOSTILES")));
        m_pStatusTextComp->SetTextRenderColor(_bActive ? FColor(30, 255, 70) : FColor(220, 20, 15));
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
        //「UUserWidget* goalWidget = CreateWidget<UUserWidget>(GetWorld(), m_goalWidgetClass)」が成立するとき、AddToViewportを呼び出します。
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

    //コンポーネントの解決に失敗した場合、処理を中断
    if (!ResolveComponents()) { return; }

    //ゴール到達がリクエストされたことを記録し、衝突を無効化
    m_bTransitionRequested = true;
    m_pSphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BP_OnGoalReached(CastChecked<APlayerChara>(_otherActor));

    //ゴール到達時の処理をスケジュール
    TWeakObjectPtr<AGoalActor> weakThis(this);
    GetWorldTimerManager().SetTimer(m_clearTimer,
                                    FTimerDelegate::CreateLambda(
                                        [weakThis]()
                                        {
                                            //「!weakThis.IsValid()」が成立するとき、Getを呼び出します。
                                            if (!weakThis.IsValid()) { return; }

                                            //goalActorは、weakThis.Get()から取得した参照を後続の呼び出しで使います。
                                            AGoalActor* goalActor = weakThis.Get();
                                            //「!goalActor->m_gameClearLevelName.IsNone()」が成立するとき、OpenLevelを呼び出します。
                                            if (!goalActor->m_gameClearLevelName.IsNone())
                                            {
                                                //Levelへ安全に遷移します。
                                                UGameplayStatics::OpenLevel(goalActor, goalActor->m_gameClearLevelName);
                                            }
                                        }),
                                    FMath::Max(0.f, m_clearDelay), false);
}
