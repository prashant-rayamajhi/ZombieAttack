#include "GameFlowUISubsystem.h"

#include "GameFlowScreenWidget.h"
#include "../Configuration/ZombieAttackUISettings.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

//Subsystemを初期化し、レベル遷移後のUI生成通知を登録します。
void UGameFlowUISubsystem::Initialize(FSubsystemCollectionBase& _collection)
{
    Super::Initialize(_collection);

    m_postLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UGameFlowUISubsystem::HandlePostLoadMap);
}

//Subsystem終了時にレベル遷移通知とWidget参照を解除します。
void UGameFlowUISubsystem::Deinitialize()
{
    if (m_postLoadMapHandle.IsValid())
    {
        FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(m_postLoadMapHandle);
        m_postLoadMapHandle.Reset();
    }
    if (m_pActiveWidget)
    {
        m_pActiveWidget->RemoveFromParent();
        m_pActiveWidget = nullptr;
    }

    Super::Deinitialize();
}

//PostLoadMapの通知を受けてゲーム状態へ反映します。
void UGameFlowUISubsystem::HandlePostLoadMap(UWorld* _loadedWorld)
{
    if (!_loadedWorld || !_loadedWorld->IsGameWorld() || _loadedWorld->GetGameInstance() != GetGameInstance()) { return; }
    const FString LevelName = UGameplayStatics::GetCurrentLevelName(_loadedWorld, true);
    if (!IsGameFlowMap(LevelName))
    {
        if (m_pActiveWidget)
        {
            m_pActiveWidget->RemoveFromParent();
            m_pActiveWidget = nullptr;
        }
        return;
    }

    //PostLoadMap直後はPlayerController生成前のことがあるため、次のTickで表示します。
    TWeakObjectPtr<UGameFlowUISubsystem> WeakThis(this);
    TWeakObjectPtr<UWorld> WeakWorld(_loadedWorld);
    _loadedWorld->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda(
        [WeakThis, WeakWorld]()
        {
            if (WeakThis.IsValid() && WeakWorld.IsValid())
            {
                WeakThis->ShowScreen(WeakWorld.Get());
            }
        }));
}

//画面を画面へ表示します。
void UGameFlowUISubsystem::ShowScreen(UWorld* _world)
{
    if (!_world || !IsGameFlowMap(UGameplayStatics::GetCurrentLevelName(_world, true))) { return; }

    //プレイヤーコントローラーを返します。
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(_world, 0);
    if (!PlayerController)
    {
        return;
    }
    TArray<UUserWidget*> ExistingWidgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, ExistingWidgets, UGameFlowScreenWidget::StaticClass(), true);
    const FString LevelName = UGameplayStatics::GetCurrentLevelName(_world, true);
    const TSubclassOf<UGameFlowScreenWidget> ScreenClass = GetDefault<UZombieAttackUISettings>()->GetGameFlowWidgetClass(LevelName);

    m_pActiveWidget = ExistingWidgets.Num() > 0 ? Cast<UGameFlowScreenWidget>(ExistingWidgets[0])
                                                : CreateWidget<UGameFlowScreenWidget>(PlayerController, ScreenClass);
    if (!m_pActiveWidget)
    {
        return;
    }
    if (!m_pActiveWidget->IsInViewport())
    {
        m_pActiveWidget->AddToViewport(100);
    }

    //UIOnly入力にはフォーカス可能なSlate Widgetが必要なため、表示後の操作先を明示します。
    m_pActiveWidget->SetIsFocusable(true);

    //入力モードを保持します。
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(m_pActiveWidget->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputMode);
    PlayerController->SetShowMouseCursor(true);
    m_pActiveWidget->FocusDefaultButton(PlayerController);
}

//GameFlowMapかを判定します。
bool UGameFlowUISubsystem::IsGameFlowMap(const FString& _levelName) const
{
    return _levelName.Equals(TEXT("GameStart"), ESearchCase::IgnoreCase) || _levelName.Equals(TEXT("GameClear"), ESearchCase::IgnoreCase) ||
           _levelName.Equals(TEXT("GameOver"), ESearchCase::IgnoreCase);
}
