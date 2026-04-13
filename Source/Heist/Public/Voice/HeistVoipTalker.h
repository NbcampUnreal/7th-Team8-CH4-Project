#pragma once

#include "CoreMinimal.h"
#include "Net/VoiceConfig.h"
#include "HeistVoipTalker.generated.h"

UCLASS()
class HEIST_API UHeistVoipTalker : public UVOIPTalker
{
	GENERATED_BODY()

public:
	virtual void OnTalkingBegin(UAudioComponent* AudioComponent) override;
	virtual void OnTalkingEnd() override;

private:
	void BroadcastTalkingState(bool bIsTalking);
};
