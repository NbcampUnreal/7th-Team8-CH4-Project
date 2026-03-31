#pragma once

#include "CoreMinimal.h"
#include "Character/HeistCharacter.h"
#include "ThiefCharacter.generated.h"

/**
 * 도둑 전용 캐릭터.
 * 게임플레이 로직은 GA와 컴포넌트에 위임한다.
 */
UCLASS()
class HEIST_API AThiefCharacter : public AHeistCharacter
{
	GENERATED_BODY()

public:
	AThiefCharacter(const FObjectInitializer& ObjectInitializer);
};
