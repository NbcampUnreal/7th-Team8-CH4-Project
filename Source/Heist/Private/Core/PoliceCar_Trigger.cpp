#include "Core/PoliceCar_Trigger.h"

#include "Character/ThiefCharacter.h"
#include "Components/ThiefEscortComponent.h"
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

	// 1. 도둑에게 경찰차 도착 이벤트 전달 (아웃 처리용)
	UAbilitySystemComponent* ThiefASC = Thief->GetAbilitySystemComponent();
	if (IsValid(ThiefASC))
	{
		ThiefASC->HandleGameplayEvent(HeistEventTags::Event_ArrivedAtCar, &Payload);
	}

	// 2. 경찰에게 이벤트 전달하여 Escort 어빌리티 정상 종료
	UAbilitySystemComponent* PoliceASC = Police->GetAbilitySystemComponent();
	if (IsValid(PoliceASC))
	{
		PoliceASC->HandleGameplayEvent(HeistEventTags::Event_ArrivedAtCar, &Payload);
	}
}
