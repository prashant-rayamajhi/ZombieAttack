#include "PlayerAnimInstance.h"
#include "ZombieAttack/Player/PlayerChara.h"

//後退の符号は移動用だけに付け、既存の待機・ジャンプ判定が使うSpeedは変更しない。
void UPlayerAnimInstance::NativeUpdateAnimation(float _deltaSeconds)
{
    Super::NativeUpdateAnimation(_deltaSeconds);
    const APlayerChara* player = Cast<APlayerChara>(TryGetPawnOwner());
    if (!player) { m_locomotionSpeed = 0.0f; return; }
    m_locomotionSpeed = player->GetMoveSpeed() * (player->GetVelocityForward() < -0.1f ? -1.0f : 1.0f);
}
