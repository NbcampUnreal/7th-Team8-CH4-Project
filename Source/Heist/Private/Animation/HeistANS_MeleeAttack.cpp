
#include "Animation/HeistANS_MeleeAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/HeistHitReactionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"

UHeistANS_MeleeAttack::UHeistANS_MeleeAttack()
{
}

void UHeistANS_MeleeAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	AlreadyHitActors.Empty();
}

void UHeistANS_MeleeAttack::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!IsValid(MeshComp)) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!IsValid(Owner) || !Owner->HasAuthority()) return; // 클라면 그냥 리턴박습니다.
	
	// 공격자의 HitReactionComponent가 melee hit 공통 진입점을 처리합니다.
	UHeistHitReactionComponent* HitReactionComp = Owner->FindComponentByClass<UHeistHitReactionComponent>();
	if (!IsValid(HitReactionComp) || !HitReactionComp->HasMeleeHitHandler()) return;

	const FVector SocketLocation = MeshComp->GetSocketLocation(HitSocket);

	// 소켓 위치 기준 구체 오버랩
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	Owner->GetWorld()->OverlapMultiByChannel(Overlaps, SocketLocation, FQuat::Identity,
		HitChannel, FCollisionShape::MakeSphere(HitSphereRadius), Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!IsValid(HitActor) || AlreadyHitActors.Contains(HitActor)) continue;

		AlreadyHitActors.Add(HitActor);
		HitReactionComp->ProcessMeleeHit(Owner, HitActor);
	}
}

void UHeistANS_MeleeAttack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	AlreadyHitActors.Empty();
}
