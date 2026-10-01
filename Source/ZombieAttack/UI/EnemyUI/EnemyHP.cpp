#include "EnemyHP.h"
#include "../../Enemy/EnemyChara.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

//体力UIを最新のゲーム状態へ更新します。
void UEnemyHP::UpdateHealthUI()
{
    if (!m_owner.IsValid() || !m_healthBar) { return; }

    //最大体力を返します。
    const float MaxHP = m_owner->GetMaxHP();
    //体力を返します。
    const float CurrentHP = m_owner->GetHP();
    m_healthBar->SetPercent(MaxHP > 0.f ? CurrentHP / MaxHP : 0.f);
    if (m_currentHealth)
    {
        m_currentHealth->SetText(FText::AsNumber(FMath::RoundToInt(CurrentHP)));
    }
    if (m_maxHealth)
    {
        m_maxHealth->SetText(FText::AsNumber(FMath::RoundToInt(MaxHP)));
    }
}
//所有者を設定します。
void UEnemyHP::SetOwner(AEnemyChara* _pEnemy) { m_owner = _pEnemy; }
