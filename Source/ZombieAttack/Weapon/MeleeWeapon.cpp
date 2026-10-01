#include "MeleeWeapon.h"

#include "../Character/BaseCharacter.h"
#include "../Player/PlayerChara.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

//AMeleeWeaponが使用するComponentと初期パラメータを設定します。
AMeleeWeapon::AMeleeWeapon()
    : m_attackRange(150.0f), m_attackRadius(50.0f), m_attackCoolDown(0.3f), m_comboWindowDuration(0.8f), m_maxComboCount(3), m_pSwingSound(nullptr),
      m_pHitSound(nullptr), m_pSwingVFX(nullptr), m_pHitVFX(nullptr), m_currentComboIndex(0), m_bIsAttacking(false), m_bComboQueued(false),
      m_bHitExecutedThisSwing(false)
{
    //魔法Auraは使用せず、命中が確定した地点だけに短い衝撃を表示します。
    m_pSwingVFX = nullptr;
    m_pHitVFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave"));
}

//Weaponを使用します。
void AMeleeWeapon::UseWeapon()
{
    if (m_bIsAttacking)
    {
        //攻撃中に入力された場合、次段へつなげる予約だけします。
        m_bComboQueued = true;
        return;
    }

    PlayCurrentComboMontage();
}

//現在のComboMontageを再生します。
void AMeleeWeapon::PlayCurrentComboMontage()
{
    //所有者キャラクターを保持します。
    ACharacter* ownerCharacter = Cast<ACharacter>(m_pOwnerChara);
    if (!ownerCharacter) { return; }

    m_bIsAttacking = true;
    m_bHitExecutedThisSwing = false;
    if (m_pSwingSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, m_pSwingSound, GetActorLocation());
    }
    //攻撃Montageを再生できたため保険処理が不要かを示します。
    bool bPlayedMontage = false;

    //攻撃アニメーションはプレイヤー本体のSkeletonで再生するため、
    //基本はPlayerChara側の m_knifeAttackMontages を使います。
    if (APlayerChara* player = Cast<APlayerChara>(ownerCharacter))
    {
        bPlayedMontage = player->PlayKnifeAttackMontage(m_currentComboIndex);
    }

    //互換用です。PlayerChara側にMontageが未設定の場合のみ、
    //以前通りBP_Melee側のMontageを使います。
    if (!bPlayedMontage)
    {
        //モンタージュを保持します。
        UAnimMontage* montage = nullptr;
        if (m_attackMontages.IsValidIndex(m_currentComboIndex))
        {
            montage = m_attackMontages[m_currentComboIndex];
        }
        if (montage)
        {
            bPlayedMontage = ownerCharacter->PlayAnimMontage(montage) > 0.0f;
        }
    }
    if (!bPlayedMontage)
    {
        //Montage未設定でもヒット確認だけできるようにします。
        ExecuteHit();
        TryContinueComboFromNotify();
    }

    GetWorldTimerManager().ClearTimer(m_comboResetTimer);
    GetWorldTimerManager().SetTimer(m_comboResetTimer, this, &AMeleeWeapon::ResetCombo, m_comboWindowDuration, false);
}

//Hitを実行します。
void AMeleeWeapon::ExecuteHit()
{
    if (m_bHitExecutedThisSwing) { return; }

    m_bHitExecutedThisSwing = true;
    //VFXは空振りでは出さず、PerformSweepで命中が確定した時だけ再生します。
    PerformSweep();
}

//攻撃MontageのNotifyを受け、入力済みの次段攻撃へ繋げます。
void AMeleeWeapon::TryContinueComboFromNotify()
{
    m_bIsAttacking = false;
    if (!m_bComboQueued) { return; }

    m_bComboQueued = false;
    const int32 nextComboIndex = m_currentComboIndex + 1;
    int32 availableComboCount = m_attackMontages.Num();
    if (const APlayerChara* player = Cast<APlayerChara>(m_pOwnerChara))
    {
        if (player->GetKnifeAttackMontageCount() > 0)
        {
            availableComboCount = player->GetKnifeAttackMontageCount();
        }
    }
    const int32 maxComboIndex = FMath::Min(FMath::Max(0, m_maxComboCount - 1), FMath::Max(0, availableComboCount - 1));
    if (nextComboIndex > maxComboIndex)
    {
        ResetCombo();
        return;
    }

    m_currentComboIndex = nextComboIndex;

    GetWorldTimerManager().SetTimer(m_cooldownTimer, this, &AMeleeWeapon::PlayCurrentComboMontage, m_attackCoolDown, false);
}

//Sweepを判定範囲へ実行します。
void AMeleeWeapon::PerformSweep()
{
    if (!m_pWeapon || !m_pOwnerChara) { return; }
    const FVector traceStart = m_pWeapon->GetSocketLocation(TEXT("WeaponSocket"));
    const FVector traceEnd = traceStart + m_pWeapon->GetForwardVector() * m_attackRange;
    FHitResult hitResult;
    FCollisionQueryParams params;
    params.AddIgnoredActor(this);

    //所有者キャラクターを保持します。
    ACharacter* ownerCharacter = Cast<ACharacter>(m_pOwnerChara);
    if (ownerCharacter)
    {
        params.AddIgnoredActor(ownerCharacter);
    }

    //ワールドを返します。
    const bool bHit =
        GetWorld()->SweepSingleByChannel(hitResult, traceStart, traceEnd, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(m_attackRadius), params);
    if (bHit && hitResult.GetActor())
    {
        if (m_pHitVFX)
        {
            //SystemAt位置を作成します。
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_pHitVFX, hitResult.ImpactPoint, hitResult.ImpactNormal.Rotation(), FVector(0.11f));
        }
        if (m_pHitSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, m_pHitSound, hitResult.ImpactPoint);
        }

        //コントローラーを返します。
        AController* ownerController = ownerCharacter ? ownerCharacter->GetController() : nullptr;
        //ダメージを処理します。
        UGameplayStatics::ApplyDamage(hitResult.GetActor(), m_damage, ownerController, this, UDamageType::StaticClass());
    }
}

//Comboを解除します。
void AMeleeWeapon::ResetCombo()
{
    m_currentComboIndex = 0;
    m_bIsAttacking = false;
    m_bComboQueued = false;
    m_bHitExecutedThisSwing = false;
}
