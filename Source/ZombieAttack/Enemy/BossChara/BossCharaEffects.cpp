#include "BossChara.h"
#include "ZombieAttack/Player/PlayerChara.h"
#include "ZombieAttack/AIController/BossAIController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"

//通常攻撃の衝撃を足元の地面へ出す。ダメージは手足の接触判定に任せる。
void ABossChara::ExecuteLightComboAreaAttack()
{
    if (m_bLightComboAreaResolvedThisStep || IsDead() || !m_bAttacking || m_bRecovering || m_bIsTransitioning) { return; }
    m_bLightComboAreaResolvedThisStep = true;

    //攻撃通知の瞬間に足元を取り直し、坂道でも衝撃波を地面に沿わせる。
    FVector effectLocation;
    FRotator effectRotation;
    ResolveGroundEffectTransform(0.0f, effectLocation, effectRotation);
    UNiagaraSystem* comboImpactVFX = m_pComboImpactVFX.Get();
    if (!comboImpactVFX)
    {
        //BP側が未設定でも、作品内の既定エフェクトで見た目が欠落しないようにします。
        comboImpactVFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave"));
    }
    if (comboImpactVFX)
    {
        //SystemAt位置を作成します。
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, comboImpactVFX, effectLocation, effectRotation, FVector(0.20f));
    }

    //範囲だけで当てず、手足の接触結果を共通の攻撃判定から受け取る。
}

//攻撃がヒットしたときの処理
void ABossChara::OnEnemyAttackHit(AActor* _hitActor)
{
    if (!_hitActor) { return; }

    //命中した瞬間だけ、攻撃種類に合うインパクトを表示する
    //Slamは接地Notifyで生成済みのため、プレイヤー位置へ二重生成しません。
    UNiagaraSystem* impactSystem = m_currentPattern == EBossAttackPattern::PowerSlam ? nullptr : m_pComboImpactVFX.Get();
    if (impactSystem)
    {
        //エフェクト位置を保持します。
        FVector effectLocation;
        //エフェクト回転を保持します。
        FRotator effectRotation;
        ResolveGroundEffectTransform(0.0f, effectLocation, effectRotation);
        const float effectScale = m_currentPattern == EBossAttackPattern::PowerSlam ? 0.52f : 0.14f;
        //SystemAt位置を作成します。
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, impactSystem, effectLocation, effectRotation, FVector(effectScale));
    }
    m_bComboHitConfirmed = true;

    //ヒット時のサウンドを再生
    if (m_pImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, m_pImpactSound, GetActorLocation());
    }

    //BossAIControllerにプレイヤーがヒットしたことを通知
    if (ABossAIController* bossAIController = Cast<ABossAIController>(GetController()))
    {
        bossAIController->NotifyPlayerHit(static_cast<float>(m_hitDamage));
    }
}

//Slamの演出だけをモンタージュ終盤に生成します。
void ABossChara::SpawnPowerSlamEffect()
{
    //死亡状態かどうかを返します。
    if (!m_bAttacking || IsDead() || m_currentPattern != EBossAttackPattern::PowerSlam || m_bPowerSlamEffectSpawned) { return; }
    UNiagaraSystem* powerSlamVFX = m_pPowerSlamVFX.Get();
    if (!powerSlamVFX)
    {
        //BPに参照が保存されていなくても、既定の衝撃波を使用します。
        powerSlamVFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave"));
    }
    if (!powerSlamVFX)
    {
        return;
    }

    //エフェクト位置を保持します。
    FVector effectLocation;
    //エフェクト回転を保持します。
    FRotator effectRotation;
    ResolveGroundEffectTransform(0.0f, effectLocation, effectRotation);

    //SystemAt位置を作成します。
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, powerSlamVFX, effectLocation, effectRotation, FVector(0.52f));

    m_bPowerSlamEffectSpawned = true;
}

//ボスの足元または指定した前方位置から、実際の地面Transformを取得します。
bool ABossChara::ResolveGroundEffectTransform(float _forwardOffset, FVector& _outLocation, FRotator& _outRotation) const
{
    const FVector traceCenter = GetActorLocation() + GetActorForwardVector() * FMath::Max(0.0f, _forwardOffset);
    return ResolveGroundEffectTransformAt(traceCenter, _outLocation, _outRotation);
}

//地面をTraceし、攻撃エフェクトの位置と傾きを接地面へ合わせます。
bool ABossChara::ResolveGroundEffectTransformAt(const FVector& _traceCenter, FVector& _outLocation, FRotator& _outRotation) const
{
    FHitResult groundHit;
    FCollisionQueryParams queryParams(SCENE_QUERY_STAT(BossGroundEffect), false, this);
    queryParams.AddIgnoredActor(this);

    //頭上の枝や他のキャラクターではなく、足元直下の静的な床だけを調べる。
    const float footZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector traceStart(_traceCenter.X, _traceCenter.Y, footZ + 50.0f);
    const FVector traceEnd(_traceCenter.X, _traceCenter.Y, footZ - 300.0f);
    FCollisionObjectQueryParams groundObjects;
    groundObjects.AddObjectTypesToQuery(ECC_WorldStatic);
    const bool bFoundGround =
        GetWorld() && GetWorld()->LineTraceSingleByObjectType(groundHit, traceStart, traceEnd, groundObjects, queryParams);
    const float fallbackGroundZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    _outLocation = bFoundGround ? groundHit.ImpactPoint + groundHit.ImpactNormal * 3.0f : FVector(_traceCenter.X, _traceCenter.Y, fallbackGroundZ);
    _outRotation = bFoundGround ? FRotationMatrix::MakeFromZ(groundHit.ImpactNormal).Rotator() : FRotator::ZeroRotator;
    return bFoundGround;
}
