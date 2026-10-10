#include "EnemyChara.h"
#include "../AIController/EnemyAIController.h"
#include "../Player/PlayerChara.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "ZombieAttack/Animation/Enemy/EnemyAnimationTiming.h"
#include "TimerManager.h"
#include "Engine/World.h"

//存在する骨へ判定を付け替え、メッシュ原点で殴る状態を防ぐ。
bool AEnemyChara::PrepareAttackContact(FName _bone)
{
    SetAttackCollisionEnabled(false);
    if (!GetMesh() || !m_pAttackCollision || !GetMesh()->DoesSocketExist(_bone)) { return false; }
    m_pAttackCollision->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, _bone);
    return true;
}

//攻撃通知が選んだ手足の接触位置を返す。
FVector AEnemyChara::GetAttackContactLocation() const
{
    return m_pAttackCollision ? m_pAttackCollision->GetComponentLocation() : GetActorLocation();
}

//攻撃ロジックを処理する
void AEnemyChara::HandleAttackLogic(float _deltaTime)
{
    //AIコントローラーを取得し、プレイヤーキャラクターが存在しないか、死亡している場合は処理を終了する
    AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController());
    if (!AIController || !m_pPlayerChara || m_pPlayerChara->IsDead() || AIController->IsAlertReactionActive()) { return; }

    //現在のAI状態を取得し、パトロールまたは探索状態の場合は移動速度をパトロール速度に設定する
    const EEnemyAIState State = AIController->GetCurrentState();
    if (State == EEnemyAIState::Patrol || State == EEnemyAIState::Search)
    {
        m_moveState = EEnemyMoveState::Patrol;
        GetCharacterMovement()->MaxWalkSpeed = m_patrolWalkSpeed;
        return;
    }

    //現在のAI状態が追跡または攻撃状態の場合、移動状態を戦闘に設定し、攻撃中でない場合は移動速度を追跡速度に設定する
    m_moveState = EEnemyMoveState::Combat;
    if (m_bIsAttacking) { return; }

    //追跡速度に設定する
    GetCharacterMovement()->MaxWalkSpeed = m_chaseRunSpeed;

    //現在のAI状態が追跡状態で、攻撃クールダウンが0以下で、プレイヤーとの距離が攻撃範囲以内の場合、攻撃を開始する
    if (State == EEnemyAIState::Chase && m_attackCooldownRemaining <= 0.f &&
        AIController->HasAttackOpening() && CanHitPlayer(m_attackRange, -1.0f))
    {
        //横を向いたまま振りかぶらず、先に足を止めて相手へ向き直る。
        AIController->StopMovement();
        GetCharacterMovement()->StopMovementImmediately();
        const FRotator facing(0.0f, (m_pPlayerChara->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0.0f);
        SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), facing, _deltaTime, 240.0f));
        if (CanHitPlayer(m_attackRange, 0.95f)) { BeginAttack(); }
    }
}

//攻撃を開始する
void AEnemyChara::BeginAttack()
{
    //攻撃中、死亡中、またはプレイヤーキャラクターが存在しない場合は処理を終了する
    if (m_bIsDead || m_bIsAttacking || !m_pPlayerChara) { return; }

    //同じ技を続けて再生しても、前回の終了通知が新しい攻撃を終了させない。
    if (UAnimInstance* previous = GetMesh()->GetAnimInstance())
    {
        if (m_activeAttack)
        {
            FOnMontageEnded ended;
            FOnMontageBlendingOutStarted blending;
            previous->Montage_SetEndDelegate(ended, m_activeAttack);
            previous->Montage_SetBlendingOutDelegate(blending, m_activeAttack);
        }
    }

    //攻撃中フラグを設定し、ヒットが適用されたかどうかのフラグをリセットする
    m_bIsAttacking = true;
    m_bHitAppliedThisAttack = false;
    m_bAttackHitThisSwing = false;

    //経路追従停止後に数フレーム残る速度を消し、停止アニメーション中の滑りを防ぎます。
    if (UCharacterMovementComponent* movementComponent = GetCharacterMovement())
    {
        movementComponent->StopMovementImmediately();
        movementComponent->MaxWalkSpeed = 0.0f;
        movementComponent->SetAvoidanceEnabled(false);
    }

    //AIコントローラーを取得し、攻撃開始を通知する
    if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
    {
        AIController->NotifyAttackStarted();
    }

    //プレイヤーの方向を向く
    const FVector ToPlayer = m_pPlayerChara->GetActorLocation() - GetActorLocation();
    SetActorRotation(FRotator(0.f, ToPlayer.Rotation().Yaw, 0.f));
    PlayEnemySound(m_pAttackSound);
    //攻撃モンタージュを再生し、再生時間を取得する
    float MontageDuration = 0.f;
    //直前と異なる攻撃を優先し、使用できる候補がなければ既存のパンチを使う。
    TArray<UAnimMontage*> choices;
    for (UAnimMontage* montage : m_attackChoices)
    {
        if (montage != m_activeAttack && IsMontageCompatible(montage) && EnemyAnimationTiming::HasHitNotify(montage)) { choices.Add(montage); }
    }
    m_activeAttack = choices.IsEmpty() ? m_pAttackMontage.Get() : choices[FMath::RandRange(0, choices.Num() - 1)];
    if (!IsMontageCompatible(m_activeAttack) || !EnemyAnimationTiming::HasHitNotify(m_activeAttack))
    {
        EndAttack();
        return;
    }

    //再生速度を変えても、元のクリップ長ではなく実際の終了時間まで足を止める。
    UAnimInstance* instance = GetMesh()->GetAnimInstance();
    const float playRate = FMath::FRandRange(0.98f, 1.12f);
    MontageDuration = instance ? instance->Montage_Play(m_activeAttack, playRate, EMontagePlayReturnType::Duration) : 0.0f;
    if (MontageDuration <= 0.0f)
    {
        EndAttack();
        return;
    }

    //終了と中断のどちらでも足止めを解除し、攻撃判定は通知区間だけで管理する。
    FOnMontageEnded endDelegate;
    endDelegate.BindUObject(this, &AEnemyChara::HandleAttackMontageEnded);
    GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(endDelegate, m_activeAttack);
    //攻撃を中断した後の補間中にも手が動くため、中断開始で判定を閉じる。
    FOnMontageBlendingOutStarted blendDelegate;
    blendDelegate.BindWeakLambda(this, [this](UAnimMontage* _montage, bool _interrupted)
    {
        if (_interrupted && _montage == m_activeAttack) { EndAttack(); }
    });
    instance->Montage_SetBlendingOutDelegate(blendDelegate, m_activeAttack);
    GetWorldTimerManager().SetTimer(m_attackEndTimer, this, &AEnemyChara::EndAttack, MontageDuration + 0.1f, false);
}

//攻撃ヒットを適用する
void AEnemyChara::PerformAttackHit(float _damageMultiplier)
{
    //古いBPや単発通知から呼ばれても、攻撃区間外や手足が届かない位置には当てない。
    if (!m_pAttackCollision || m_pAttackCollision->GetCollisionEnabled() == ECollisionEnabled::NoCollision) { return; }
    if (!m_pPlayerChara || !m_pAttackCollision->IsOverlappingActor(m_pPlayerChara)) { return; }
    //攻撃中でない、ヒットがすでに適用されている、または死亡している場合は処理を終了する
    if (!m_bIsAttacking || m_bHitAppliedThisAttack || m_bIsDead) { return; }

    //ヒットが適用されたことをフラグで記録し、攻撃ヒットタイマーをクリアする
    m_bHitAppliedThisAttack = true;

    //プレイヤーキャラクターが存在しない、または死亡している場合は処理を終了する
    if (!m_pPlayerChara || m_pPlayerChara->IsDead()) { return; }

    //プレイヤーとの距離が攻撃範囲以内の場合、ダメージを適用する
    if (CanHitPlayer(m_attackRange))
    {
        m_bAttackHitThisSwing = true;
        //ダメージを処理します。
        UGameplayStatics::ApplyDamage(m_pPlayerChara, static_cast<float>(m_hitDamage) * FMath::Max(0.f, _damageMultiplier), GetController(), this,
                                      UDamageType::StaticClass());
        OnEnemyAttackHit(m_pPlayerChara);
    }
}

//攻撃を終了する
void AEnemyChara::EndAttack()
{
    //攻撃中でない場合は処理を終了する
    if (!m_bIsAttacking) { return; }

    //攻撃中フラグをリセットし、攻撃クールダウンをリセットする
    m_bIsAttacking = false;
    SetAttackCollisionEnabled(false);
    EndAttackVFXWindow();
    //同時に生成された敵でも、次に踏み込む間隔を個別に変える。
    m_attackCooldownRemaining = m_attackInterval * FMath::FRandRange(0.8f, 1.4f);
    GetWorldTimerManager().ClearTimer(m_attackEndTimer);
    if (m_bIsDead) { return; }

    //次の追跡要求を受ける前に、停止していた移動速度を元へ戻します。
    if (UCharacterMovementComponent* movementComponent = GetCharacterMovement())
    {
        movementComponent->MaxWalkSpeed = m_chaseRunSpeed;
        movementComponent->SetAvoidanceEnabled(true);
    }

    //AIコントローラーを取得し、攻撃終了を通知する
    if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
    {
        AIController->NotifyAttackFinished();
    }
}

//攻撃判定の有効化/無効化を設定する
void AEnemyChara::SetAttackCollisionEnabled(bool _bEnabled)
{
    m_contactReady = false;
    if (!m_pAttackCollision) { return; }
    _bEnabled = _bEnabled && !m_bIsDead && m_bIsAttacking;

    //攻撃判定の衝突設定を更新し、オーバーラップイベントの生成を有効化/無効化する
    m_pAttackCollision->SetCollisionEnabled(_bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    m_pAttackCollision->SetGenerateOverlapEvents(_bEnabled);
}

//手足の箱を前回位置から掃引し、通知区間の接触だけを命中処理へ渡す。
void AEnemyChara::TraceAttackContact()
{
    if (!m_pAttackCollision || m_pAttackCollision->GetCollisionEnabled() == ECollisionEnabled::NoCollision) { return; }
    if (m_bIsDead || !m_bIsAttacking || m_bHitAppliedThisAttack) { return; }
    //骨の付け替えやテレポートを長距離の攻撃として判定しない。
    const FVector end = GetAttackContactLocation();
    const FVector start = m_contactReady && FVector::DistSquared(m_previousContact, end) < 90000.f ? m_previousContact : end;
    m_previousContact = end;
    m_contactReady = true;
    //プレイヤーのカプセルに接触した結果を集め、壁越しの命中は視線判定で除外する。
    TArray<FHitResult> hits;
    FCollisionQueryParams query(SCENE_QUERY_STAT(EnemyAttackContact), false, this);
    const FCollisionObjectQueryParams objects(ECC_Pawn);
    const FCollisionShape shape = FCollisionShape::MakeBox(m_pAttackCollision->GetScaledBoxExtent());
    GetWorld()->SweepMultiByObjectType(hits, start, end, m_pAttackCollision->GetComponentQuat(), objects, shape, query);
    for (const FHitResult& hit : hits)
    {
        OnAttackCollisionOverlap(m_pAttackCollision, hit.GetActor(), hit.GetComponent(), 0, true, hit);
    }
}

//新しい攻撃スイングのために攻撃ヒットフラグをリセットする
void AEnemyChara::ResetAttackHitForNewSwing()
{
    m_bAttackHitThisSwing = false;
    m_bHitAppliedThisAttack = false;
}

//攻撃判定がプレイヤーと重なった場合の処理
void AEnemyChara::OnAttackCollisionOverlap(UPrimitiveComponent* _overlappedComp, AActor* _otherActor, UPrimitiveComponent* _otherComp,
                                           int32 _otherBodyIndex, bool _bFromSweep, const FHitResult& _sweepResult)
{
    //攻撃ヒットがすでに適用されている、または死亡している場合は処理を終了する
    if (m_bAttackHitThisSwing || m_bHitAppliedThisAttack || IsDead() || !m_bIsAttacking) { return; }
    if (!m_pAttackCollision || m_pAttackCollision->GetCollisionEnabled() == ECollisionEnabled::NoCollision) { return; }

    //重なったアクターがプレイヤーキャラクターでない場合は処理を終了する
    APlayerChara* player = Cast<APlayerChara>(_otherActor);
    if (!player || player->IsDead() || !CanHitPlayer(m_attackRange)) { return; }

    //攻撃ヒットが適用されたことをフラグで記録する
    m_bAttackHitThisSwing = true;
    m_bHitAppliedThisAttack = true;

    //プレイヤーにダメージを適用する
    UGameplayStatics::ApplyDamage(player, static_cast<float>(m_hitDamage), GetController(), this, UDamageType::StaticClass());

    //ヒットイベントを通知する
    OnEnemyAttackHit(player);
}

//攻撃ヒットイベントを通知する（通常敵は何もしない）
void AEnemyChara::OnEnemyAttackHit(AActor* _hitActor)
{
    //通常敵の攻撃はアニメーションと被弾画面演出だけで伝えます。
    //魔法的な汎用Impact VFXは世界観と接触点に合わないため表示しません。
}

//アニメーション通知からコンボ分岐を要求する（通常敵はコンボ分岐なし）
void AEnemyChara::RequestComboBranchFromNotify()
{
    //通常敵はコンボ分岐なしです。
    SetAttackCollisionEnabled(false);
    //分岐通知はモーションの途中に置かれるため、移動の再開は終了通知へ任せる
}

//再生が中断された場合も攻撃中の停止を解除し、次の追跡判断へ戻す
void AEnemyChara::HandleAttackMontageEnded(UAnimMontage* _montage, bool _bInterrupted)
{
    if (_montage == m_activeAttack) { EndAttack(); }
}

//カプセル間の距離、上下差、向き、遮蔽物を揃えて近接攻撃の届く範囲を求める
bool AEnemyChara::CanHitPlayer(float _range, float _minFacing) const
{
    if (!m_pPlayerChara || m_pPlayerChara->IsDead() || GetEffectiveDistanceToPlayer() > _range) { return false; }
    const FVector offset = m_pPlayerChara->GetActorLocation() - GetActorLocation();
    if (FMath::Abs(offset.Z) > 150.0f) { return false; }
    if (FVector::DotProduct(GetActorForwardVector(), offset.GetSafeNormal2D()) < _minFacing) { return false; }
    const AAIController* controller = Cast<AAIController>(GetController());
    return controller && controller->LineOfSightTo(m_pPlayerChara);
}
