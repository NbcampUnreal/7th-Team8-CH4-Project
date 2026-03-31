#include "Input/HeistInputConfig.h"

#include "InputAction.h"

const UInputAction* UHeistInputConfig::FindNativeActionByTag(const FGameplayTag& InputTag) const
{
	for (const FHeistInputMapping& Mapping : NativeInputMappings)
	{
		if (Mapping.InputTag == InputTag && IsValid(Mapping.InputAction))
		{
			return Mapping.InputAction;
		}
	}
	return nullptr;
}
