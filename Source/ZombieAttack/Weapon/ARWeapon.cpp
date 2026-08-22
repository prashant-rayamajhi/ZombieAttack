#include "ARWeapon.h"

//AARWeaponが使用するComponentと初期パラメータを設定します。
AARWeapon::AARWeapon()
{
    m_maxClipAmmo = 30;
    m_totalMaxAmmo = 120;
    m_currentClipAmmo = 30;
    m_currentTotalAmmo = 0;
    m_fireRate = 0.1f;
}

//弾薬を現在の所持内容へ追加します。
void AARWeapon::AddAmmo(float _amount)
{
    //弾薬を現在の所持内容へ追加します。
    Super::AddAmmo(_amount);
}
