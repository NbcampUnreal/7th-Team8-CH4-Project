#include "Animation/AnimNotify_HeistFootstep.h"

#include "Character/ThiefCharacter.h"
#include "Character/PoliceCharacter.h"
#include "Character/HeistTags_State.h"
#include "Systems/Audio/HeistAudioSubsystem.h"

#include "AbilitySystemComponent.h"
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

	AHeistCharacter* HeistCharacter = Cast<AHeistCharacter>(OwnerActor);
	if (IsValid(HeistCharacter))
	{
		UAbilitySystemComponent* ASC = HeistCharacter->GetAbilitySystemComponent();
		if (IsValid(ASC) && ASC->HasMatchingGameplayTag(HeistStateTags::State_Sneaking)) return;
	}

	if (AThiefCharacter* ThiefCharacter = Cast<AThiefCharacter>(OwnerActor))
	{
		AudioSubsystem->PlayOneShotSound(EHeistSoundType::Footstep_Thief, MeshComp->GetComponentLocation());
		ThiefCharacter->ReportFootstep();
	}
	else if (APoliceCharacter* PoliceCharacter = Cast<APoliceCharacter>(OwnerActor))
	{
		AudioSubsystem->PlayOneShotSound(EHeistSoundType::Footstep_Police, MeshComp->GetComponentLocation());
	}
}
