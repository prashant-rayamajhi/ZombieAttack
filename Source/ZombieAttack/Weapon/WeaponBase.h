#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

//ABaseCharacterは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class ABaseCharacter;
//USkeletalMeshComponentは、ポインターまたは参照の型解決に必要な宣言だけを先行して用意します。
class USkeletalMeshComponent;

//武器Baseの動作をまとめたクラス
UCLASS(Abstract)
class ZOMBIEATTACK_API AWeaponBase : public AActor
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    AWeaponBase();

    //UseWeaponは、名前が示す装備または機能を使用する処理を開始します。
    virtual void UseWeapon();
    //AddAmmoは、名前が示す対象を既存の状態または一覧へ追加します。
    virtual void AddAmmo(float _amount);
    //所有者キャラクターを設定します。
    void SetOwnerCharacter(ABaseCharacter* _pNewOwner);

    //武器メッシュを返します。
    USkeletalMeshComponent* GetWeaponMesh() const { return m_pWeapon; }
    //所有者キャラクターを返します。
    ABaseCharacter* GetOwnerCharacter() const { return m_pOwnerChara; }

  protected:
    //武器を保持します。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    TObjectPtr<USkeletalMeshComponent> m_pWeapon;

    //ダメージを保持します。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0"))
    float m_damage;

    //所有者キャラクターを保持します。
    UPROPERTY(Transient)
    TObjectPtr<ABaseCharacter> m_pOwnerChara;
};
