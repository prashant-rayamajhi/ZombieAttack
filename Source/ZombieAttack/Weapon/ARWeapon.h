#pragma once

#include "CoreMinimal.h"
#include "GunWeapon.h"
#include "ARWeapon.generated.h"

//ARWeaponの動作をまとめたクラス
UCLASS()
class ZOMBIEATTACK_API AARWeapon : public AGunWeapon
{
    //エンジンが使う定型コード
    GENERATED_BODY()

  public:
    AARWeapon();
    //AddAmmoは、名前が示す対象を既存の状態または一覧へ追加します。
    virtual void AddAmmo(float _amount) override;
};
