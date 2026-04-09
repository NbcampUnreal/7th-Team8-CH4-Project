// Fill out your copyright notice in the Description page of Project Settings.

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
	
	// 공격자의 HitReactionComponent에 캐싱해놓은 타격 로직을 가져옵니다.
	UHeistHitReactionComponent* HitReactionComp = Owner->FindComponentByClass<UHeistHitReactionComponent>();
	if (!IsValid(HitReactionComp) || !HitReactionComp->OnMeleeHit.IsBound()) return;

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
		
		// 여기서, 피격자에게 공격자 데이터를 담아 보냅니다
		FGameplayEventData Payload;
		Payload.Instigator = Owner;
		Payload.Target     = HitActor;
		HitReactionComp->OnMeleeHit.Execute(Payload);
	}
}

void UHeistANS_MeleeAttack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	AlreadyHitActors.Empty();
}