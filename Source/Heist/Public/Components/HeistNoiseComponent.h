#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistNoiseComponent.generated.h"

class UDataTable;
class UAbilitySystemComponent;
class UAudioComponent;
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
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Audio")
	TObjectPtr<UDataTable> SoundDataTable;

	UPROPERTY()
	TObjectPtr<UAudioComponent> ActiveChannelingAudio;

	UFUNCTION(Server, Reliable)
	void Server_StartChannelingNoise(EHeistSoundType SoundType);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartChannelingNoise(EHeistSoundType SoundType);

	UFUNCTION(Server, Reliable)
	void Server_StopChannelingNoise();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopChannelingNoise();

	// 1. 서버 권한으로 실제 로직을 처리하는 함수
	UFUNCTION(Server, Reliable)
	void Server_ProcessNoise(EHeistSoundType SoundType, FVector OriginLocation);

	// 2. 모든 클라이언트에게 오디오 재생을 지시하는 함수 (엔진 자체 감쇠 적용)
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlaySound(EHeistSoundType SoundType, FVector OriginLocation);

	// 내부 헬퍼 함수들
	const FHeistSoundData* GetSoundData(EHeistSoundType SoundType) const;
	float CalculateFinalDetectionRadius(const FHeistSoundData* SoundData, UAbilitySystemComponent* OwnerASC) const;
	void DetectPoliceAndSendEvent(const FHeistSoundData* SoundData, float FinalRadius, FVector OriginLocation);
};
