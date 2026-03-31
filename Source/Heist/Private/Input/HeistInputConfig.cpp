#include "Input/HeistInputConfig.h"

#include "InputAction.h"

const UInputAction* UHeistInputConfig::FindInputActionByTag(const FGameplayTag& InputTag) const
{
	for (const FHeistInputMapping& Mapping : InputMappings)
	{
		if (Mapping.InputTag == InputTag && IsValid(Mapping.InputAction))
		{
			return Mapping.InputAction;
		}
	}
	return nullptr;
}
