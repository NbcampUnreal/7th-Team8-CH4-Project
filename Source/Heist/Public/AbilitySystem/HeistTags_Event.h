#pragma once

#include "NativeGameplayTags.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "HeistTags_Event.generated.h"

namespace HeistEventTags
{
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit);
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Healed);
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_CuffingComplete);
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_EscortStarted);
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_EscortInterrupted);
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_ArrivedAtCar);
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Arrested); 	// 경찰차 도착 후 서버가 체포를 확정했을 때 후속 GAS/BP 반응용으로 사용하는 이벤트
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_KickHit);
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_StunExpired);
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Input_Move);
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_SoundDetected);
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_CarryStarted);
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_CarryUpdate);
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_CarryDrop);
	HEIST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_EngineStarted);
}

/*
 * GAS에서 여러 값을 이벤트로 전달하는 공식 패턴 - TargetData *
 */
USTRUCT()
struct FHeistKickPayload : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	float KnockbackDistance = 0.f;
	float StunDuration = 0.f; // 0 이하 = Knockback Only

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FHeistKickPayload::StaticStruct();
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << KnockbackDistance;
		Ar << StunDuration;
		bOutSuccess = true;
		return true;
	}
};

// 직렬화 설정같네요..
template<>
struct TStructOpsTypeTraits<FHeistKickPayload>
	: public TStructOpsTypeTraitsBase2<FHeistKickPayload>
{
	enum { WithNetSerializer = true };
};
