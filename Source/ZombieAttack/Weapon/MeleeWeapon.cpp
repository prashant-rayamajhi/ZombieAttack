#include "MeleeWeapon.h"

#include "../Character/BaseCharacter.h"
#include "../Player/PlayerChara.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

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
    //次段への待ち時間中は新しい一段目を始めず、予約済みの一撃だけを再生する。
    if (GetWorldTimerManager().IsTimerActive(m_cooldownTimer)) { return; }
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
    //装備変更や死亡で取り消したコンボのタイマーを、別の行動へ持ち越さない。
    GetWorldTimerManager().ClearTimer(m_cooldownTimer);
    //所有者キャラクターを保持します。
    ACharacter* ownerCharacter = Cast<ACharacter>(m_pOwnerChara);
    if (!ownerCharacter) { return; }
    if (const APlayerChara* player = Cast<APlayerChara>(ownerCharacter))
    {
        if (player->IsDead() || player->GetCurrentWeapon() != this)
        {
            ResetCombo();
            return;
        }
    }

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

    //長い斬撃を固定の0.8秒で解除せず、実際の再生終了まで次段入力と命中通知を受け付ける。
    float resetDelay = FMath::Max(0.01f, m_comboWindowDuration);
    UAnimInstance* instance = ownerCharacter->GetMesh() ? ownerCharacter->GetMesh()->GetAnimInstance() : nullptr;
    UAnimMontage* montage = instance ? instance->GetCurrentActiveMontage() : nullptr;
    if (bPlayedMontage && montage)
    {
        const float rate = FMath::Abs(instance->Montage_GetPlayRate(montage) * montage->RateScale);
        resetDelay = FMath::Max(resetDelay, montage->GetPlayLength() / FMath::Max(0.01f, rate) + 0.05f);
    }
    GetWorldTimerManager().ClearTimer(m_comboResetTimer);
    GetWorldTimerManager().SetTimer(m_comboResetTimer, this, &AMeleeWeapon::ResetCombo, resetDelay, false);
}

//Hitを実行します。
void AMeleeWeapon::ExecuteHit()
{
    //中断した攻撃の通知が遅れて届いても、待機中や装備変更後には命中させない。
    if (!m_bIsAttacking || m_bHitExecutedThisSwing) { return; }
    if (const APlayerChara* player = Cast<APlayerChara>(m_pOwnerChara))
    {
        if (player->IsDead() || player->GetCurrentWeapon() != this) { return; }
    }

    m_bHitExecutedThisSwing = true;
    //VFXは空振りでは出さず、PerformSweepで命中が確定した時だけ再生します。
    PerformSweep();
}

//攻撃MontageのNotifyを受け、入力済みの次段攻撃へ繋げます。
void AMeleeWeapon::TryContinueComboFromNotify()
{
    //同じ分岐通知が重複しても、次段の予約を作り直さない。
    if (!m_bIsAttacking) { return; }
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

    //前段のリセットが次段の開始前に発火すると、予約された攻撃が一段目へ巻き戻るため解除する。
    GetWorldTimerManager().ClearTimer(m_comboResetTimer);
    GetWorldTimerManager().SetTimer(m_cooldownTimer, this, &AMeleeWeapon::PlayCurrentComboMontage, FMath::Max(0.01f, m_attackCoolDown), false);
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
        GetWorld()->SweepSingleByChannel(
            hitResult, traceStart, traceEnd, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(m_attackRadius), params);
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
    //状態だけでなく次段の再生予約も取り消し、武器をしまった後の攻撃を防ぐ。
    GetWorldTimerManager().ClearTimer(m_comboResetTimer);
    GetWorldTimerManager().ClearTimer(m_cooldownTimer);
    m_currentComboIndex = 0;
    m_bIsAttacking = false;
    m_bComboQueued = false;
    m_bHitExecutedThisSwing = false;
}
