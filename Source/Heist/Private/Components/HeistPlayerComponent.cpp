#include "Components/HeistPlayerComponent.h"

#include "Components/HeistPawnExtensionComponent.h"
#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "Input/HeistInputComponent.h"
#include "Input/HeistInputConfig.h"
#include "Input/HeistTags_Input.h"
#include "Data/HeistPawnData.h"
#include "Data/HeistTags_InitState.h"

#include "Components/GameFrameworkComponentManager.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"

const FName UHeistPlayerComponent::NAME_ActorFeatureName("Player");

UHeistPlayerComponent::UHeistPlayerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

UHeistPlayerComponent* UHeistPlayerComponent::FindPlayerComponent(const AActor* Actor)
{
	if (!IsValid(Actor)) return nullptr;
	return Actor->FindComponentByClass<UHeistPlayerComponent>();
}

void UHeistPlayerComponent::BeginPlay()
{
	Super::BeginPlay();

	RegisterInitStateFeature();

	// PawnExtension의 모든 상태 변화를 구독 — 변화 시 OnActorInitStateChanged 호출
	BindOnActorInitStateChanged(UHeistPawnExtensionComponent::NAME_ActorFeatureName,
		FGameplayTag(), false);

	// BeginPlay 시점에 이미 조건이 충족된 경우를 처리
	CheckDefaultInitialization();
}

void UHeistPlayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}

void UHeistPlayerComponent::OnPawnInputComponentReady(UInputComponent* InputComponent)
{
	bInputComponentReady = true;
	CheckDefaultInitialization();
}

bool UHeistPlayerComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	if (!CurrentState.IsValid() && DesiredState == HeistInitStateTags::InitState_GameplayReady)
	{
		return bInputComponentReady
			&& Manager->HasFeatureReachedInitState(GetOwner(),
				UHeistPawnExtensionComponent::NAME_ActorFeatureName,
				HeistInitStateTags::InitState_GameplayReady);
	}

	return false;
}

void UHeistPlayerComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (DesiredState == HeistInitStateTags::InitState_GameplayReady)
	{
		BindInput();
	}
}

void UHeistPlayerComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == UHeistPawnExtensionComponent::NAME_ActorFeatureName)
	{
		CheckDefaultInitialization();
	}
}

void UHeistPlayerComponent::CheckDefaultInitialization()
{
	TryToChangeInitState(HeistInitStateTags::InitState_GameplayReady);
}

void UHeistPlayerComponent::BindInput()
{
	if (bInputBound) return;

	APawn* Pawn = GetPawn<APawn>();
	if (!IsValid(Pawn)) return;

	APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
	if (!IsValid(PlayerController)) return;

	UHeistPawnExtensionComponent* PawnExtension = UHeistPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!IsValid(PawnExtension)) return;

	const UHeistPawnData* PawnData = PawnExtension->GetPawnData();
	if (!IsValid(PawnData)) return;

	const UHeistInputConfig* InputConfig = PawnData->InputConfig;
	if (!IsValid(InputConfig)) return;

	UHeistInputComponent* HeistInputComp = Cast<UHeistInputComponent>(Pawn->InputComponent);
	if (!IsValid(HeistInputComp)) return;

	// IMC 등록
	if (IsValid(PawnData->DefaultMappingContext))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Subsystem->AddMappingContext(PawnData->DefaultMappingContext, PawnData->DefaultMappingPriority);
			}
		}
	}

	// 이동 — 직접 핸들러 바인딩
	HeistInputComp->BindActionByTag(InputConfig, HeistInputTags::Input_Move,
		ETriggerEvent::Triggered, this, &UHeistPlayerComponent::HandleMoveInput);

	// 어빌리티 입력 — InputConfig 전체를 순회하여 태그 기반으로 ASC에 라우팅
	HeistInputComp->BindAbilityActions(InputConfig,
		this,
		&UHeistPlayerComponent::HandleAbilityInputTagPressed,
		&UHeistPlayerComponent::HandleAbilityInputTagReleased,
		AbilityInputBindHandles);

	bInputBound = true;
}

void UHeistPlayerComponent::HandleMoveInput(const FInputActionValue& Value)
{
	APawn* Pawn = GetPawn<APawn>();
	if (!IsValid(Pawn)) return;

	// 이동 입력이 0이 아닐 때 ASC로 이동 이벤트 전달
	const FVector2D MoveVector = Value.Get<FVector2D>();
	if (!MoveVector.IsNearlyZero())
	{
		UHeistPawnExtensionComponent* PawnExtension = UHeistPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
		if (IsValid(PawnExtension))
		{
			UHeistAbilitySystemComponent* ASC = PawnExtension->GetAbilitySystemComponent();
			if (IsValid(ASC))
			{
				FGameplayEventData Payload;
				Payload.Instigator = Pawn;

				ASC->HandleGameplayEvent(HeistEventTags::Event_Input_Move, &Payload);
			}
		}
	}
	Pawn->AddMovementInput(FVector::ForwardVector, MoveVector.Y);
	Pawn->AddMovementInput(FVector::RightVector, MoveVector.X);
}

void UHeistPlayerComponent::HandleAbilityInputTagPressed(FGameplayTag InputTag)
{
	UHeistPawnExtensionComponent* PawnExtension = UHeistPawnExtensionComponent::FindPawnExtensionComponent(GetPawn<APawn>());
	if (!IsValid(PawnExtension)) return;

	UHeistAbilitySystemComponent* ASC = PawnExtension->GetAbilitySystemComponent();
	if (!IsValid(ASC)) return;

	ASC->AbilityInputTagPressed(InputTag);
}

void UHeistPlayerComponent::HandleAbilityInputTagReleased(FGameplayTag InputTag)
{
	UHeistPawnExtensionComponent* PawnExtension = UHeistPawnExtensionComponent::FindPawnExtensionComponent(GetPawn<APawn>());
	if (!IsValid(PawnExtension)) return;

	UHeistAbilitySystemComponent* ASC = PawnExtension->GetAbilitySystemComponent();
	if (!IsValid(ASC)) return;

	ASC->AbilityInputTagReleased(InputTag);
}
