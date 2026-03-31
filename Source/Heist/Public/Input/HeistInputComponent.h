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

		const UInputAction* InputAction = InputConfig->FindNativeActionByTag(InputTag);
		if (!IsValid(InputAction)) return;

		BindAction(InputAction, TriggerEvent, Object, Func);
	}

	// InputConfig의 모든 매핑을 순회하며 어빌리티 입력을 태그 기반으로 바인딩한다.
	// PressedFunc / ReleasedFunc: void(FGameplayTag) 시그니처
	template<class UserClass, typename FuncType>
	void BindAbilityActions(const UHeistInputConfig* InputConfig,
		UserClass* Object, FuncType PressedFunc, FuncType ReleasedFunc,
		TArray<uint32>& OutBindHandles)
	{
		if (!IsValid(InputConfig)) return;

		for (const FHeistInputMapping& Mapping : InputConfig->AbilityInputMappings)
		{
			if (!IsValid(Mapping.InputAction) || !Mapping.InputTag.IsValid()) continue;

			OutBindHandles.Add(BindAction(Mapping.InputAction, ETriggerEvent::Started,
				Object, PressedFunc, Mapping.InputTag).GetHandle());
			OutBindHandles.Add(BindAction(Mapping.InputAction, ETriggerEvent::Completed,
				Object, ReleasedFunc, Mapping.InputTag).GetHandle());
		}
	}
};
