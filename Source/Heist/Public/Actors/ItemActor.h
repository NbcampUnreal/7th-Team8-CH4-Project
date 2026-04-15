#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/HeistCarryable.h"
#include "ItemActor.generated.h"

class USceneComponent;
class UBoxComponent;
class UStaticMeshComponent;
class AHeistCharacter;
class UHeistInteractSphereComponent;
struct FGameplayTag;

UCLASS()
class HEIST_API AItemActor : public AActor, public IHeistCarryable
{
	GENERATED_BODY()
	
public:	
	AItemActor();

	UFUNCTION(BlueprintCallable, Category = "Heist|Item")
	void OnPickedUp(AHeistCharacter* InCarrier);

	UFUNCTION(BlueprintCallable, Category = "Heist|Item")
	void OnDropOff(AHeistCharacter* InCarrier);

	UFUNCTION(BlueprintCallable, Category = "Heist|Item")
	virtual int32 GetRequiredCarriers_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Heist|Item")
	virtual float GetCarrySpeedMultiplier_Implementation(int32 CarrierCount) const override;

	UFUNCTION(BlueprintCallable, Category = "Heist|Item")
	int32 GetCurrentCarrierCount() { return CurrentCarrierCount; }

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 데이터 테이블 기반 초기화
	void InitializeFromData();

	UFUNCTION(BlueprintNativeEvent, Category = "Heist|Item")
	void OnExplode();

	const struct FItemData* GetItemData() const;

	void CheckDrop();

	bool CheckCanInteract(ACharacter* Interactor) const;

	FGameplayTag ResolveInteractAbilityTag(ACharacter* Interactor) const;

	void NotifyCarriersUpdate();

	void DropCarrier(AHeistCharacter* Carrier);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<UBoxComponent> BoxCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistInteractSphereComponent> InteractSphereComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	FDataTableRowHandle ItemData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TMap<AHeistCharacter*, FRotator> CurrentCarriers;

	UPROPERTY(Replicated)
	int32 CurrentCarrierCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float CarryDistance = 120.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float MaxFollowSpeed = 1000.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float DropAngleMax = 90.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float CarryDistanceMax = 200.f;

	FTimerHandle PhysicsTimeoutHandle;
};
