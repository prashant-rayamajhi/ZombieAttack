#include "PlayerAudioComponent.h"

#include "../../Weapon/GunWeapon.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

//プレイヤー音声コンポーネントを処理します。
UPlayerAudioComponent::UPlayerAudioComponent()
    : m_pFootstepSound(nullptr), m_pLandingSound(nullptr), m_minimumFootstepSpeed(40.0f), m_minimumFootstepInterval(0.08f),
      m_pReloadAudioComponent(nullptr), m_lastFootstepTime(-DBL_MAX)
{
    PrimaryComponentTick.bCanEverTick = false;
    static ConstructorHelpers::FObjectFinder<USoundBase> FootstepSound(TEXT("/Game/Audio/CC0/S_Player_Footstep.S_Player_Footstep"));
    static ConstructorHelpers::FObjectFinder<USoundBase> LandingSound(TEXT("/Game/Audio/CC0/S_Player_Land.S_Player_Land"));

    m_pFootstepSound = FootstepSound.Object;
    m_pLandingSound = LandingSound.Object;
}

//足音を再生します。
void UPlayerAudioComponent::PlayFootstep()
{
    if (!CanPlayFootstep() || !m_pFootstepSound) { return; }

    //所有者を返します。
    AActor* owner = GetOwner();
    //ワールドを返します。
    UWorld* world = GetWorld();
    if (!owner || !world) { return; }

    m_lastFootstepTime = world->GetTimeSeconds();

    //効果音Attachedを作成します。
    UGameplayStatics::SpawnSoundAttached(m_pFootstepSound, owner->GetRootComponent(), NAME_None, FVector::ZeroVector,
                                         EAttachLocation::KeepRelativeOffset, true, 1.0f, FMath::FRandRange(0.96f, 1.04f));

    UAISense_Hearing::ReportNoiseEvent(world, owner->GetActorLocation(), 0.3f, owner, 1200.0f, TEXT("Footstep"));
}

//Landingを再生します。
void UPlayerAudioComponent::PlayLanding()
{
    if (!m_pLandingSound || !GetOwner()) { return; }

    UGameplayStatics::PlaySoundAtLocation(this, m_pLandingSound, GetOwner()->GetActorLocation());
}

//リロード音を再生します。
void UPlayerAudioComponent::PlayReloadSound(AGunWeapon* _gunWeapon, float _targetDuration)
{
    StopReloadSound(0.0f);
    if (!IsValid(_gunWeapon) || _targetDuration <= KINDA_SMALL_NUMBER) { return; }

    //リロード効果音を返します。
    USoundBase* reloadSound = _gunWeapon->GetReloadSound();
    if (!reloadSound) { return; }
    const float soundDuration = reloadSound->GetDuration();
    const float pitchMultiplier = soundDuration > KINDA_SMALL_NUMBER
                                      //値が範囲を超えないように収めます。
                                      ? FMath::Clamp(soundDuration / _targetDuration, 0.25f, 3.0f)
                                      : 1.0f;

    m_pReloadAudioComponent =
        //効果音Attachedを作成します。
        UGameplayStatics::SpawnSoundAttached(reloadSound, _gunWeapon->GetRootComponent(), NAME_None, FVector::ZeroVector,
                                             EAttachLocation::KeepRelativeOffset, true, 1.0f, pitchMultiplier);
}

//Montage終了に合わせてリロード音を停止します。
void UPlayerAudioComponent::StopReloadSound(float _fadeOutDuration)
{
    if (!IsValid(m_pReloadAudioComponent))
    {
        m_pReloadAudioComponent = nullptr;
        return;
    }
    if (_fadeOutDuration > 0.0f)
    {
        m_pReloadAudioComponent->FadeOut(_fadeOutDuration, 0.0f);
    }
    else
    {
        m_pReloadAudioComponent->Stop();
    }

    m_pReloadAudioComponent = nullptr;
}

//移動状態と接地状態から足音を鳴らせるか判定します。
bool UPlayerAudioComponent::CanPlayFootstep() const
{
    //所有者を返します。
    const ACharacter* character = Cast<ACharacter>(GetOwner());
    //ワールドを返します。
    const UWorld* world = GetWorld();
    if (!character || !world) { return false; }

    //キャラクター移動を返します。
    const UCharacterMovementComponent* movement = character->GetCharacterMovement();
    if (!movement || !movement->IsMovingOnGround() || character->GetVelocity().Size2D() < m_minimumFootstepSpeed) { return false; }

    return world->GetTimeSeconds() - m_lastFootstepTime >= m_minimumFootstepInterval;
}
