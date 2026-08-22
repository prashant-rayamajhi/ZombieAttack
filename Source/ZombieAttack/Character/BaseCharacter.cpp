#include "BaseCharacter.h"
#include "ZombieAttack/Weapon/WeaponBase.h"

//コンストラクタ
ABaseCharacter::ABaseCharacter() : m_pCurrentWeapon(nullptr), m_MaxHp(100.f), m_Hp(100.f), m_Damage(10.f), m_bDeathEventBroadcast(false)
{
    //Tickを無効化
    PrimaryActorTick.bCanEverTick = false;
}

//ゲーム開始時に呼ばれる関数
void ABaseCharacter::BeginPlay()
{
    //親クラスのBeginPlayを呼び出す
    Super::BeginPlay();

    //最大HPを1以上に制限し、現在のHPを最大HPの範囲内にクランプする
    m_MaxHp = FMath::Max(1.f, m_MaxHp);
    m_Hp = FMath::Clamp(m_Hp, 0.f, m_MaxHp);
}

//ダメージを受けたときの処理
float ABaseCharacter::TakeDamage(float _damageAmount, const FDamageEvent& _damageEvent, AController* _eventInstigator, AActor* _damageCauser)
{
    //「_damageAmount <= 0.f」が成立するとき、この関数を終了します。
    if (_damageAmount <= 0.f) { return 0.f; }

    return FMath::Max(0.f, Super::TakeDamage(_damageAmount, _damageEvent, _eventInstigator, _damageCauser));
}

//攻撃処理
void ABaseCharacter::Attack()
{
    //現在の武器が有効であれば、武器の使用処理を呼び出す
    if (IsValid(m_pCurrentWeapon))
    {
        m_pCurrentWeapon->UseWeapon();
    }
}

//死亡イベントを一度だけブロードキャストする
void ABaseCharacter::BroadcastDeathOnce()
{
    //すでに死亡イベントがブロードキャストされている場合は処理を終了
    if (m_bDeathEventBroadcast) { return; }

    //死亡イベントをブロードキャストする
    m_bDeathEventBroadcast = true;
    OnCharacterDied.Broadcast(this);
}

//死亡処理
void ABaseCharacter::Die()
{
    //死亡イベントを一度だけブロードキャストする
    BroadcastDeathOnce();
    Destroy();
}
