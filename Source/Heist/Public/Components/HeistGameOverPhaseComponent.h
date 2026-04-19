#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistGameOverPhaseComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HEIST_API UHeistGameOverPhaseComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHeistGameOverPhaseComponent();

		
};
