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
class UHeistTransparencyComponent;
struct FGameplayTag;

USTRUCT()
struct FCarrierEntry
{
	GENERATED_BODY()

	UPROPERTY()
	AHeistCharacter* Carrier = nullptr;

	UPROPERTY()
	FRotator StartRotator = FRotator::ZeroRotator;
};

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

	UFUNCTION(BlueprintCallable, Category = "Heist|Item")
	int32 GetItemValue();

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 데이터 테이블 기반 초기화
	void InitializeFromData();

	UFUNCTION()
	void OnRep_CurrentCarrierCount();

	UFUNCTION()
	void OnRep_CarrierEntries();

	const struct FItemData* GetItemData() const;

	float GetGroundZ(const FVector& AtLocation) const;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Vision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistTransparencyComponent> TransparencyComponent;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentCarrierCount)
	int32 CurrentCarrierCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float CarryDistance = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float DropAngleMax = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float CarryDistanceMax = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float CarryDistanceMin = 50.f;

	UPROPERTY(ReplicatedUsing = OnRep_CarrierEntries)
	TArray<FCarrierEntry> ReplicatedCarriers;
};
