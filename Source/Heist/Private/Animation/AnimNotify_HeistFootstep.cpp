#include "Animation/AnimNotify_HeistFootstep.h"

#include "Character/ThiefCharacter.h"
#include "Character/PoliceCharacter.h"
#include "Systems/Audio/HeistAudioSubsystem.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

void UAnimNotify_HeistFootstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp)) return;

	UWorld* World = MeshComp->GetWorld();
	if (!IsValid(World)) return;

	UHeistAudioSubsystem* AudioSubsystem = World->GetSubsystem<UHeistAudioSubsystem>();
	if (!IsValid(AudioSubsystem)) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!IsValid(OwnerActor)) return;

	if (AThiefCharacter* ThiefCharacter = Cast<AThiefCharacter>(OwnerActor))
	{
		AudioSubsystem->PlayOneShotSound(EHeistSoundType::Footstep_Thief, MeshComp->GetComponentLocation());
		ThiefCharacter->ReportFootstep();
	}
	// TODO(하민): 경찰은 AnimNotify조차 호출되지 않고 있는 문제 해결
	else if (APoliceCharacter* PoliceCharacter = Cast<APoliceCharacter>(OwnerActor))
	{
		AudioSubsystem->PlayOneShotSound(EHeistSoundType::Footstep_Police, MeshComp->GetComponentLocation());
	}
}
