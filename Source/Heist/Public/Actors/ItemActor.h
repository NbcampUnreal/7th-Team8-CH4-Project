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

	// 물리 이벤트 발생 시 호출 (서버 -> 전체 클라이언트)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnItemPhysicsEvent(FVector ImpulseDir, float Force);

	UFUNCTION(BlueprintCallable, Category = "Heist|Item")
	void OnPickedUp(AHeistCharacter* InCarrier);

	UFUNCTION(BlueprintCallable, Category = "Heist|Item")
	int32 GetRequiredCarriers() const;

	UFUNCTION(BlueprintCallable, Category = "Heist|Item")
	float GetCarrySpeedMultiplier() const;

	UFUNCTION(BlueprintCallable, Category = "Heist|Item")
	float GetSoloCarrySpeedMultiplier() const;

protected:
	virtual void BeginPlay() override;

	// 데이터 테이블 기반 초기화
	void InitializeFromData();

	UFUNCTION(BlueprintNativeEvent, Category = "Heist|Item")
	void OnExplode();

	const struct FItemData* GetItemData() const;

	// 운반 인원 확인 및 페널티 계산 (서버 실행)
	void UpdateCarryingState(const TArray<ACharacter*>& CurrentCarriers);

	// 물리 종료 및 위치 확정 타이머
	void FinalizePhysicsLocation();

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	FDataTableRowHandle ItemData;

	bool bIsCarried;
	bool bIsSoloCarried;

	FTimerHandle PhysicsTimeoutHandle;
};
