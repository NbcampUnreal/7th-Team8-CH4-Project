#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/HeistInteractable.h"
#include "Interaction/HeistCarryable.h"
#include "ItemActor.generated.h"

class USceneComponent;
class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class HEIST_API AItemActor : public AActor, public IHeistInteractable, public IHeistCarryable
{
	GENERATED_BODY()
	
public:	
	AItemActor();

protected:
	virtual void BeginPlay() override;

	// --- IHeistInteractable Implementation ---
	virtual bool CanInteract_Implementation(ACharacter* Interactor) const override;
	virtual FGameplayTag GetInteractAbilityTag_Implementation(ACharacter* Interactor) const override;
	virtual float GetInteractRadius_Implementation() const override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
private:
	UFUNCTION()
	void OnRep_IsCarried();

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<UBoxComponent> BoxCollision;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	FDataTableRowHandle ItemData;

	UPROPERTY(ReplicatedUsing = OnRep_IsCarried)
	bool bIsCarried;
};
