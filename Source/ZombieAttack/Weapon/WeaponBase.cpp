#include "WeaponBase.h"
#include "../Character/BaseCharacter.h"
#include "Components/SkeletalMeshComponent.h"

//AWeaponBaseが使用するComponentと初期パラメータを設定します。
AWeaponBase::AWeaponBase() : m_pWeapon(nullptr), m_Damage(20.f), m_pOwnerChara(nullptr)
{
    PrimaryActorTick.bCanEverTick = false;
    m_pWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    SetRootComponent(m_pWeapon);
    m_pWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

//Weaponを使用します。
void AWeaponBase::UseWeapon() {}

//所有者キャラクターを設定します。
void AWeaponBase::SetOwnerCharacter(ABaseCharacter* NewOwner)
{
    m_pOwnerChara = NewOwner;
    SetOwner(NewOwner);
    SetInstigator(NewOwner);
}

//弾薬を現在の所持内容へ追加します。
void AWeaponBase::AddAmmo(float Amount) {}
