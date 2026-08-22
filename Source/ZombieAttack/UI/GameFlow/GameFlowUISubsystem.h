#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFlowUISubsystem.generated.h"

//UGameFlowScreenWidgetは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class UGameFlowScreenWidget;

//画面用マップを読み込んだときに、共通UIを自動生成します。
//Level BlueprintのCreate Widget設定がなくてもタイトル画面が成立します。
UCLASS()
class ZOMBIEATTACK_API UGameFlowUISubsystem : public UGameInstanceSubsystem
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //Initializeは、Initializeの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    virtual void Initialize(FSubsystemCollectionBase& _collection) override;
    //Deinitializeは、Deinitializeの名前で定義されたクラス固有の動作を実行し、その結果を呼び出し元へ反映します。
    virtual void Deinitialize() override;

  private:
    //HandlePostLoadMapは、名前が示すイベントを受け取り、その結果をゲーム状態へ反映します。
    void HandlePostLoadMap(UWorld* _loadedWorld);
    //ShowScreenは、名前が示すUIを構築して画面へ表示します。
    void ShowScreen(UWorld* _world);
    //IsGameFlowMapは、名前が示す条件の成立可否を呼び出し元へ返します。
    bool IsGameFlowMap(const FString& _levelName) const;

  private:
    //ostLoadMapHandleの操作に使用する参照です。
    FDelegateHandle m_postLoadMapHandle;

    //ActiveWidgetの操作に使用する参照です。
    UPROPERTY(Transient)
    //ActiveWidgetの操作に使用する参照です。
    TObjectPtr<UGameFlowScreenWidget> m_pActiveWidget;
};
