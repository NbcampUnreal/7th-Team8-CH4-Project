#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistNoiseComponent.generated.h"

class UDataTable;
class UAbilitySystemComponent;
struct FHeistSoundData;

enum class EHeistSoundType : uint8;

UCLASS(ClassGroup = (Heist), meta = (BlueprintSpawnableComponent))
class HEIST_API UHeistNoiseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeistNoiseComponent();

	UFUNCTION(BlueprintCallable, Category = "Heist|Audio")
	void MakeHeistNoise(EHeistSoundType SoundType, FVector OriginLocation);

	UFUNCTION(BlueprintCallable, Category = "Heist|Audio")
	void StartChannelingNoise(EHeistSoundType SoundType);

	UFUNCTION(BlueprintCallable, Category = "Heist|Audio")
	void StopChannelingNoise();

protected:
	UFUNCTION(Server, Reliable)
	void Server_StartChannelingNoise(EHeistSoundType SoundType);

	UFUNCTION(Server, Reliable)
	void Server_StopChannelingNoise();

	UFUNCTION(Server, Reliable)
	void Server_ProcessNoise(EHeistSoundType SoundType, FVector OriginLocation);

	const FHeistSoundData* GetSoundData(EHeistSoundType SoundType) const;
	float CalculateFinalDetectionRadius(const FHeistSoundData* SoundData, UAbilitySystemComponent* OwnerASC) const;
	void DetectPoliceAndSendEvent(const FHeistSoundData* SoundData, float FinalRadius, FVector OriginLocation);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Audio")
	TObjectPtr<UDataTable> SoundDataTable;

	float LastFootstepTime = 0.0f;

	static constexpr float FootstepDebounceTime = 0.15f;
};
