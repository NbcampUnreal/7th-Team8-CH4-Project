#include "UI/TestNoiseActor.h"

#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "TimerManager.h"
#include "Components/SphereComponent.h"

ATestNoiseActor::ATestNoiseActor()
{
	PrimaryActorTick.bCanEverTick = false;

	USphereComponent* SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->InitSphereRadius(50.0f);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = SphereComp;
}

void ATestNoiseActor::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(TestTimerHandle, this, &ThisClass::FireTestNoise, 3.0f, true);
}

void ATestNoiseActor::FireTestNoise()
{
	FHeistSoundDetectedMessage Message;
	Message.OriginLocation = GetActorLocation();
	Message.DetectionRadius = 1000.0f;

	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(HeistMessageTags::Message_UI_SoundDetected, Message);
}
