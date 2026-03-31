#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "Input/HeistInputConfig.h"
#include "HeistInputComponent.generated.h"

UCLASS()
class HEIST_API UHeistInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	// InputTag에 매핑된 InputAction을 찾아 바인딩한다.
	// FuncType: void() 또는 void(const FInputActionValue&) 등 EnhancedInput이 지원하는 시그니처
	template<class UserClass, typename FuncType>
	void BindActionByTag(const UHeistInputConfig* InputConfig, const FGameplayTag& InputTag,
		ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func)
	{
		if (!IsValid(InputConfig)) return;

		const UInputAction* InputAction = InputConfig->FindInputActionByTag(InputTag);
		if (!IsValid(InputAction)) return;

		BindAction(InputAction, TriggerEvent, Object, Func);
	}
};
