#include "Components/ThiefEscortComponent.h"

#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "Character/HeistCharacter.h"
#include "Character/ThiefCharacter.h"

#include "Net/UnrealNetwork.h"

UThiefEscortComponent::UThiefEscortComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(false);
	SetIsReplicatedByDefault(true);
}

void UThiefEscortComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UThiefEscortComponent, EscortedBy);
}

bool UThiefEscortComponent::BeginEscort(AHeistCharacter* InPolice, TSubclassOf<UGameplayEffect> EscortedEffectClass,
	UAbilitySystemComponent* SourceASC)
{
	AActor* OwnerActor = GetOwner();
	AThiefCharacter* OwnerThief = Cast<AThiefCharacter>(OwnerActor);
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority() || !IsValid(OwnerThief) || !IsValid(InPolice))
	{
		// 서버 아니면 False
		return false;
	}
	
	UAbilitySystemComponent* TargetASC = OwnerThief->GetAbilitySystemComponent();
	if (!IsValid(TargetASC)) return false;
	
	// 이미 Escort 중이면 중복해서 시작을 방지 - (Escorting GA에서 1차로 걸러주긴 합니다. 방어코드용)
	if (IsValid(EscortedBy)) return false;
	
	if (IsValid(EscortedEffectClass) && IsValid(SourceASC))
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EscortedEffectClass, 1.f, Context);
		if (Spec.IsValid() && Spec.Data.IsValid())
		{
			EscortedEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	SetEscortedBy(InPolice);
	return true;
}

void UThiefEscortComponent::InterruptEscort(UAbilitySystemComponent* SourceASC, bool bConvertToCuffed)
{
	InterruptEscort(DefaultCuffedEffectClass, SourceASC, bConvertToCuffed);
}

void UThiefEscortComponent::InterruptEscort(TSubclassOf<UGameplayEffect> InCuffedEffectClass,
	UAbilitySystemComponent* SourceASC, bool bConvertToCuffed)
{
	AActor* OwnerActor = GetOwner();
	AThiefCharacter* OwnerThief = Cast<AThiefCharacter>(OwnerActor);
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority() || !IsValid(OwnerThief)) return;
	
	UAbilitySystemComponent* TargetASC = OwnerThief->GetAbilitySystemComponent();
	if (!IsValid(TargetASC))
	{
		SetEscortedBy(nullptr);
		return;
	}
	
	// Escorted GE 제거
	if (EscortedEffectHandle.IsValid())
	{
		TargetASC->RemoveActiveGameplayEffect(EscortedEffectHandle);
		EscortedEffectHandle.Invalidate();
	}
	
	// 정책 분기 - bConvertedToCuff (Default : true)
	// 수갑 채운 상태로 풀려날지 아닐지 정한다
	if (bConvertToCuffed && InCuffedEffectClass && SourceASC)
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(InCuffedEffectClass, 1.f, Context);
		if (Spec.IsValid() && Spec.Data.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
		
	SetEscortedBy(nullptr);
}

bool UThiefEscortComponent::IsEscortedBy(const AHeistCharacter* InPolice) const
{
	return IsValid(InPolice) && EscortedBy == InPolice;
}

AThiefCharacter* UThiefEscortComponent::FindEscortedThiefByPolice(const AHeistCharacter* InPolice)
{
	if (!IsValid(InPolice)) return nullptr;
	
	UWorld* World = InPolice->GetWorld();
	if (!IsValid(World)) return nullptr;
	
	for (TActorIterator<AThiefCharacter> It(World); It; ++It)
	{
		AThiefCharacter* Thief = *It;
		if (!IsValid(Thief)) continue;
		
		UThiefEscortComponent* EscortComp = Thief->GetThiefEscortComponent();
		if (IsValid(EscortComp) && EscortComp->IsEscortedBy(InPolice))
		{
			return Thief;
		}
	}
	
	return nullptr;
}

void UThiefEscortComponent::SetEscortedBy(AHeistCharacter* InPolice)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority()) return;
	
	EscortedBy = InPolice;
	SetComponentTickEnabled(IsValid(InPolice));
}

void UThiefEscortComponent::OnRep_EscortedBy()
{
	SetComponentTickEnabled(IsValid(EscortedBy));
}

void UThiefEscortComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !IsValid(EscortedBy)) return;

	FVector TargetLocation = EscortedBy->GetActorLocation() - (EscortedBy->GetActorForwardVector() * EscortOffsetDistance);
	TargetLocation.Z = EscortedBy->GetActorLocation().Z;

	OwnerActor->SetActorRotation(EscortedBy->GetActorRotation());
	OwnerActor->SetActorLocation(TargetLocation, false);

}
