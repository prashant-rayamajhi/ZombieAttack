#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFlowScene.generated.h"
class UCameraComponent;
class USkeletalMeshComponent;
class UPointLightComponent;
class USpotLightComponent;
class UAnimSequence;
//本編の森とプレイヤーを使う、メニュー専用の動く背景。
UCLASS()
class ZOMBIEATTACK_API AGameFlowScene : public AActor
{
    GENERATED_BODY()
public:
    //背景モデルと専用カメラを作り、必要な素材をパッケージの参照に含める。
    AGameFlowScene();
    //開始、夜明け、敗北で灯りとプレイヤーの姿勢を変える。
    void SetScene(int32 _scene);
    //検証用の撮影でもメニューと同じ画角を使う。
    UCameraComponent* GetCamera() const { return m_camera; }
    //読みやすさを損なわない範囲でカメラと照明を動かす。
    virtual void Tick(float _deltaTime) override;
private:
    //画面ごとの視点へ短く寄り、人物を見失わない範囲でカメラを動かす。
    void UpdateCamera();
    //腕を広げる待機クリップ後半を使わず、呼吸と小さな重心移動だけを滑らかにつなぐ。
    void UpdateIdlePose();
    //タイトルで使う警戒姿勢の開始秒。戦闘用の元クリップは変更しない。
    UPROPERTY(EditDefaultsOnly, Category = "Presentation|Idle", meta = (ClampMin = "0.0"))
    float m_idleStart = 0.8f;
    //腕を広げる動作へ入る前に折り返す、タイトル専用の終了秒。
    UPROPERTY(EditDefaultsOnly, Category = "Presentation|Idle", meta = (ClampMin = "0.0"))
    float m_idleEnd = 2.8f;
    //開始姿勢へ戻るまでの秒数。端では再生速度を落として姿勢の跳びを防ぐ。
    UPROPERTY(EditDefaultsOnly, Category = "Presentation|Idle", meta = (ClampMin = "1.0"))
    float m_idleCycle = 8.0f;

    //クリア時だけ再生する、無事に脱出できたことを伝える動作。
    UPROPERTY()
    TObjectPtr<UAnimSequence> m_clearAnimation;

    //開始は人物の寄り、クリアは手の動き、敗北は倒れた体を収めるカメラの基準位置。
    FVector m_cameraHome = FVector(-310, -170, 140);
    //メニュー左側の余白を残しながら人物へ向ける注視点。
    FVector m_cameraFocus = FVector(40, -45, 105);

    //顔と装備を照らし、場面に合わせて白色と朝日の色を切り替える人物専用ライト。
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USpotLightComponent> m_portraitLight;

    //画面左にボタンの余白を残す視点。
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> m_camera;
    //ゲーム中の操作とは切り離して、待機と敗北の動作を見せるプレイヤー。
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USkeletalMeshComponent> m_player;
    //開始画面で繰り返す警戒姿勢。専用動作が未設定の画面でも代わりに使う。
    UPROPERTY()
    TObjectPtr<UAnimSequence> m_idle;
    //敗北画面で一度だけ再生する倒れる動作。
    UPROPERTY()
    TObjectPtr<UAnimSequence> m_death;
    //顔と上半身を照らす暖色の灯り。
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPointLightComponent> m_lamp;
    //カメラ移動を滑らかに進める経過秒数。
    float m_time = 0;
    //敗北時だけ警告灯を明滅させる画面種別。
    int32 m_scene = 0;
};
