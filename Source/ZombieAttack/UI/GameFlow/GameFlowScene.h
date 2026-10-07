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
    //顔と装備を照らし、場面に合わせて白色と朝日の色を切り替える人物専用ライト。
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USpotLightComponent> m_portraitLight;

    //画面左にボタンの余白を残す視点。
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> m_camera;
    //ゲーム中の操作とは切り離して、待機と敗北の動作を見せるプレイヤー。
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USkeletalMeshComponent> m_player;
    //開始画面とクリア画面で繰り返す待機動作。
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
