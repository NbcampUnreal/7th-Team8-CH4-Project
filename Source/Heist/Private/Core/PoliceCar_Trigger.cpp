#include "Core/PoliceCar_Trigger.h"

#include "Character/ThiefCharacter.h"
#include "Components/HeistArrestVictoryComponent.h"
#include "Components/ThiefEscortComponent.h"
#include "Systems/Audio/HeistAudioSubsystem.h"
#include "Core/HeistMatchGameMode.h"
#include "AbilitySystem/HeistTags_Event.h"

#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"

APoliceCar_Trigger::APoliceCar_Trigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;

	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

void APoliceCar_Trigger::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APoliceCar_Trigger::OnOverlapBegin);
	}

	if (GetNetMode() != NM_DedicatedServer)
	{
		if (UWorld* World = GetWorld())
		{
			if (UHeistAudioSubsystem* AudioSubsystem = World->GetSubsystem<UHeistAudioSubsystem>())
			{
				AudioSubsystem->PlayLoopingSound(EHeistSoundType::PoliceCar, RootComponent);
			}
		}
	}
}

void APoliceCar_Trigger::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	AThiefCharacter* Thief = Cast<AThiefCharacter>(OtherActor);
	if (!IsValid(Thief)) return;

	UThiefEscortComponent* EscortComp = Thief->GetThiefEscortComponent();
	if (!IsValid(EscortComp)) return;

	// 도둑이 이송 중인 상태일 때만 반응
	AHeistCharacter* Police = EscortComp->GetEscortedBy();
	if (!IsValid(Police)) return;

	FGameplayEventData Payload;
	Payload.Instigator = this;
	Payload.Target = Thief;

	// 1. 도둑 쪽은 네이티브 체포 처리와 분리된 연출/확장용 이벤트만 발송한다.
	UAbilitySystemComponent* ThiefASC = Thief->GetAbilitySystemComponent();
	if (IsValid(ThiefASC))
	{
		ThiefASC->HandleGameplayEvent(HeistEventTags::Event_Arrested, &Payload);
	}

	// 2. 경찰 Escort 어빌리티는 C++에서 Event_ArrivedAtCar를 기다리고 있으므로 유지한다.
	UAbilitySystemComponent* PoliceASC = Police->GetAbilitySystemComponent();
	if (IsValid(PoliceASC))
	{
		PoliceASC->HandleGameplayEvent(HeistEventTags::Event_ArrivedAtCar, &Payload);
	}

	// 체포 확정과 승리 집계는 별도의 서버 authoritative 로직에서 처리한다.
	AHeistMatchGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AHeistMatchGameMode>() : nullptr;
	if (!IsValid(GM)) return;

	if (UHeistArrestVictoryComponent* ArrestComp = GM->FindComponentByClass<UHeistArrestVictoryComponent>())
	{
		ArrestComp->NotifyThiefArrested(Thief);
	}
}
