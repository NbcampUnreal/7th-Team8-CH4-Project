#include "Components/HeistPlayerComponent.h"

#include "Components/HeistPawnExtensionComponent.h"
#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "Input/HeistInputComponent.h"
#include "Input/HeistInputConfig.h"
#include "Input/HeistTags_Input.h"
#include "Data/HeistPawnData.h"
#include "Data/HeistTags_InitState.h"

#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"

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

	UHeistPawnExtensionComponent* PawnExtension = UHeistPawnExtensionComponent::FindPawnExtensionComponent(GetPawn<APawn>());
	if (!IsValid(PawnExtension)) return;

	PawnExtension->OnGameplayReady.AddUObject(this, &UHeistPlayerComponent::OnGameplayReady);

	// BeginPlay 이전에 이미 GameplayReady에 도달한 경우를 처리
	if (PawnExtension->HasReachedInitState(HeistInitStateTags::InitState_GameplayReady))
	{
		OnGameplayReady();
	}
}

void UHeistPlayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UHeistPlayerComponent::OnPawnInputComponentReady(UInputComponent* InputComponent)
{
	bInputComponentReady = true;
	TryBindInput();
}

void UHeistPlayerComponent::OnGameplayReady()
{
	bGameplayReady = true;
	TryBindInput();
}

void UHeistPlayerComponent::TryBindInput()
{
	if (bInputBound || !bInputComponentReady || !bGameplayReady) return;
	BindInput();
}

void UHeistPlayerComponent::BindInput()
{
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

	const FVector2D MoveVector = Value.Get<FVector2D>();
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
