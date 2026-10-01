#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

//デリゲート宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDamaged);
//死亡キャラクターを通知するデリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDied, ABaseCharacter*, DeadCharacter);

//前方宣言
class AWeaponBase;

//Baseキャラクターの動作をまとめたクラス
UCLASS(Abstract)
class ZOMBIEATTACK_API ABaseCharacter : public ACharacter
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    //コンストラクタ
    ABaseCharacter();

    //ダメージを受けたときの処理
    virtual float TakeDamage(float _damageAmount, const FDamageEvent& _damageEvent, AController* _eventInstigator,
                             //overrideの操作に使用する参照です。
                             AActor* _damageCauser) override;

    //攻撃処理
    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void Attack();

    //ダメージ
    UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnDamaged m_onDamaged;

    //m_onCharacterDiedをゲーム処理から参照できるように管理します。
    UPROPERTY(BlueprintAssignable, Category = "Health")
    //m_onCharacterDiedをゲーム処理から参照できるように管理します。
    FOnDied m_onCharacterDied;

  protected:
    //開始時の処理
    virtual void BeginPlay() override;

    //死亡時の処理
    virtual void Die();

    //死亡イベントを一度だけブロードキャストする
    void BroadcastDeathOnce();

    //デフォルト武器のクラスを設定するプロパティ
    UPROPERTY(EditDefaultsOnly, Category = "Weapons")
    TSubclassOf<AWeaponBase> m_defaultWeapon;

    //現在の武器を保持するプロパティ
    UPROPERTY(Transient)
    TObjectPtr<AWeaponBase> m_pCurrentWeapon;

    //最大HPを設定するプロパティ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HP", meta = (ClampMin = "1.0"))
    float m_maxHp;

    //現在のHPを保持するプロパティ
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HP")
    float m_hp;

    //ダメージ量を設定するプロパティ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0.0"))
    float m_damage;

  private:
    //死亡イベントがすでにブロードキャストされたかどうかを追跡するフラグ
    bool m_bDeathEventBroadcast;
};
