#include "Animation/AnimNotify_HeistFootstep.h"

#include "Character/ThiefCharacter.h"

#include "Components/SkeletalMeshComponent.h"


void UAnimNotify_HeistFootstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp)) return;

	AThiefCharacter* ThiefCharacter = Cast<AThiefCharacter>(MeshComp->GetOwner());
	if (IsValid(ThiefCharacter))
	{
		ThiefCharacter->ReportFootstep();
	}
}
