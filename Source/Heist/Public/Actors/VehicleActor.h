#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VehicleActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class AHeistCharacter;
class UHeistInteractSphereComponent;
struct FGameplayTag;

UCLASS()
class HEIST_API AVehicleActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AVehicleActor();

	void SetDoorOpened(bool bIsDoorOpened);

	UFUNCTION(BlueprintCallable)
	void SetDoorMoving(bool bIsDoorMoving);

	bool GetDoorOpened() { return bDoorOpened; }

	UFUNCTION(BlueprintCallable)
	bool GetDoorMoving() { return bDoorMoving; }

	UFUNCTION(BlueprintImplementableEvent)
	void OpenDoor();

	UFUNCTION(BlueprintImplementableEvent)
	void CloseDoor();

protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool CheckCanInteract(ACharacter* Interactor) const;
	FGameplayTag ResolveInteractAbilityTag(ACharacter* Interactor) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistInteractSphereComponent> InteractSphereComponent;

	UPROPERTY(Replicated)
	bool bDoorOpened = true;

	UPROPERTY(Replicated)
	bool bDoorMoving = false;

public:
	UPROPERTY(EditAnywhere, Category = "Heist|Escape")
	int32 EscapeGroupIndex = 0;
};
