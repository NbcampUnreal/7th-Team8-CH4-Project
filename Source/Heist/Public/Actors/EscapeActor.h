#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EscapeActor.generated.h"

class USceneComponent;
class UBoxComponent;
class UStaticMeshComponent;
class AHeistCharacter;
class UHeistInteractSphereComponent;
struct FGameplayTag;

UCLASS()
class HEIST_API AEscapeActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AEscapeActor();

protected:
	virtual void BeginPlay() override;

	bool CheckCanInteract(ACharacter* Interactor) const;
	FGameplayTag ResolveInteractAbilityTag(ACharacter* Interactor) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<UBoxComponent> BoxCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistInteractSphereComponent> InteractSphereComponent;
};
