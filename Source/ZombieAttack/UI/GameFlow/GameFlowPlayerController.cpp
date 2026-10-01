#include "GameFlowPlayerController.h"
#include "GameFlowScene.h"
#include "EngineUtils.h"

#include "GameFlowScreenWidget.h"
#include "../Configuration/ZombieAttackUISettings.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

//ゲーム開始時の初期設定を行います。
void AGameFlowPlayerController::BeginPlay()
{
    //ゲーム開始時に必要な参照を取得し、初期状態を整えます。
    Super::BeginPlay();

    GetWorldTimerManager().SetTimerForNextTick(this, &AGameFlowPlayerController::EnsureGameFlowScreen);
}

//GameFlow画面が利用できる状態を保証します。
void AGameFlowPlayerController::EnsureGameFlowScreen()
{
    //専用カメラへ切り替え、UIの背後に本編と同じ森を映す。
    for (TActorIterator<AGameFlowScene> scene(GetWorld()); scene; ++scene) { SetViewTarget(*scene); break; }
    TArray<UUserWidget*> existingWidgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, existingWidgets, UGameFlowScreenWidget::StaticClass(), true);

    UGameFlowScreenWidget* screenWidget =
        existingWidgets.Num() > 0 ? Cast<UGameFlowScreenWidget>(existingWidgets[0])
                                  : CreateWidget<UGameFlowScreenWidget>(this, GetDefault<UZombieAttackUISettings>()->GetGameFlowWidgetClass(
                                                                                  UGameplayStatics::GetCurrentLevelName(this, true)));
    if (!screenWidget)
    {
        return;
    }
    if (!screenWidget->IsInViewport())
    {
        screenWidget->AddToViewport(100);
    }

    screenWidget->SetIsFocusable(true);
    //入力モードを保持します。
    FInputModeUIOnly inputMode;
    inputMode.SetWidgetToFocus(screenWidget->TakeWidget());
    inputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(inputMode);
    SetShowMouseCursor(true);
    screenWidget->FocusDefaultButton(this);

}
