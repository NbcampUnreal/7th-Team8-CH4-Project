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
#include "AbilitySystem/HeistTags_Ability.h"
#include "AbilitySystem/HeistTags_FlagTags.h"
#include "Components/HeistInteractionComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/HeistInteractable.h"
#include "Character/HeistTags_State.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "Heist/Heist.h"
#include "Systems/TransparencyTriggerReceiver.h"

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

void UHeistPlayerComponent::NotifyOverlappingTransparencyTriggers()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!IsValid(Pawn) || !Pawn->IsLocallyControlled())
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	Pawn->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		if (Actor->GetClass()->ImplementsInterface(UTransparencyTriggerReceiver::StaticClass()))
		{
			ITransparencyTriggerReceiver::Execute_EvaluateTransparencyForActor(Actor, Pawn);
		}
	}
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
	
	// 상호작용 - 이동과 마찬가지로 직접 핸들러 바인드합니다.
	HeistInputComp->BindActionByTag(InputConfig, HeistInputTags::Input_Interact,
		ETriggerEvent::Started, this, &UHeistPlayerComponent::HandleInteractPressed);
	HeistInputComp->BindActionByTag(InputConfig, HeistInputTags::Input_Interact,
	ETriggerEvent::Completed, this, &UHeistPlayerComponent::HandleInteractReleased);

	// State.MoveDisabled 태그 상태 구독 - 이동 차단 처리
	UHeistAbilitySystemComponent* ASC = PawnExtension->GetAbilitySystemComponent();
	if (IsValid(ASC))
	{
		ASC->RegisterGameplayTagEvent(
			HeistStateTags::State_MoveDisabled,
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &UHeistPlayerComponent::OnMoveDisabledTagChanged);
	}

	bInputBound = true;
}

void UHeistPlayerComponent::OnMoveDisabledTagChanged(const FGameplayTag Tag, int32 Count)
{
	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	if (!IsValid(CharacterOwner)) return;

	UCharacterMovementComponent* MoveComp = CharacterOwner->GetCharacterMovement();
	if (!IsValid(MoveComp)) return;
	
	// MoveDisabled는 이동 입력 차단용 태그
	// MovementMode 자체를 끄면 RootMotion/Knockback 같은 외부 이동까지 막히므로
	// 현재 프레임에 누적된 입력만 비워서 즉시 멈추게 한다.
	if (Count > 0)
	{
		MoveComp->StopMovementImmediately();
		CharacterOwner->ConsumeMovementInputVector();
	}
}

void UHeistPlayerComponent::HandleMoveInput(const FInputActionValue& Value)
{
	APawn* Pawn = GetPawn<APawn>();
	if (!IsValid(Pawn)) return;

	UHeistPawnExtensionComponent* PawnExtension = UHeistPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!IsValid(PawnExtension)) return;

	UHeistAbilitySystemComponent* ASC = PawnExtension->GetAbilitySystemComponent();
	if (!IsValid(ASC)) return;

	if (ASC->HasMatchingGameplayTag(HeistStateTags::State_MoveDisabled))
	{
		return;
	}

	// 이동 입력이 0이 아닐 때 ASC로 이동 이벤트 전달
	const FVector2D MoveVector = Value.Get<FVector2D>();
	if (!MoveVector.IsNearlyZero())
	{
		FGameplayEventData Payload;
		Payload.Instigator = Pawn;

		ASC->HandleGameplayEvent(HeistEventTags::Event_Input_Move, &Payload);
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

void UHeistPlayerComponent::HandleInteractPressed()
{
	// 클라이언트에서 라인트레이스 수행하고, 결과만 서버에 던지도록 구현합니다.
	APawn* Pawn = GetPawn<APawn>();
	if (!IsValid(Pawn)) return;

	// 이녀석은 어빌리티 시스템을 간접 참조하기 위해 사용, 초기화할때 PawnExtensionComponent에 캐싱됨
	UHeistPawnExtensionComponent* PawnExtension = UHeistPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!IsValid(PawnExtension)) return;

	UHeistAbilitySystemComponent* ASC = PawnExtension->GetAbilitySystemComponent();
	if (!IsValid(ASC)) return;

	// 상자 운반 미리 대응 코드 - 이미 들고있음
	if (ASC->HasMatchingGameplayTag(HeistFlagTags::Tag_Carrying)) return;
	
	// Line Trace
	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	if (!IsValid(PC)) return;
	
	FVector WorldLocation, WorldDirection;
	// 2D 마우스 방향에서 World Direction으로 반환해주는 함수로 보임 
	if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection)) return;
	
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Pawn);
	
	// 클라이언트 측에서 라인트레이스 해줘도 충분하다, 상호작용 처리만 서버에서
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
	WorldLocation,
	WorldLocation + WorldDirection * 10000.f,
		HEIST_ECC_Interact,
		Params
	);

	// 마우스 레이캐스트 디버그 라인 추가
	// DrawDebugLine(GetWorld(), WorldLocation, WorldLocation + WorldDirection * 10000.f, bHit ? FColor::Green : FColor::Red, false, 2.f, 0, 2.f);
	//
	// if (bHit && IsValid(HitResult.GetActor()))
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("Hit Actor: %s"), *HitResult.GetActor()->GetName()));
	// }

	if (!bHit || !IsValid(HitResult.GetActor())) return;
	
	// 액터 본인이 구현했거나, 산하 ActorComponent 중 하나라도 인터랙터블을 구현했는지 범용적으로 체크합니다.
	bool bIsInteractable = HitResult.GetActor()->Implements<UHeistInteractable>();
	if (!bIsInteractable)
	{
		const TArray<UActorComponent*> InteractComps = HitResult.GetActor()->GetComponentsByInterface(UHeistInteractable::StaticClass());
		if (InteractComps.Num() > 0)
		{
			bIsInteractable = true;
		}
	}
	
	if (!bIsInteractable) return; 
	
	// 마우스를 클릭한 "내 캐릭터(Pawn)"가 상호작용 목록을 추적하는 기능(HeistInteractionComponent)을 가지고 있는지 확인합니다.
	// (위의 Interactable 검사는 '맞은 액터'를 검사한 것이고, 이것은 '나'를 검사하는 것이므로 중복이 아닙니다)
	UHeistInteractionComponent* InteractComp = Pawn->FindComponentByClass<UHeistInteractionComponent>();
	if (!IsValid(InteractComp)) return;

	// 인터렉티브 Sphere 안에 있고, 레이저 쏴서 그 액터도 인터렉티브 가능하면? -> 인터렉티브 Interface 상속한 액터 코드로 가서 'Tag' 만 때온다
	FGameplayTag AbilityTag = InteractComp->ResolveInteractAbilityTag(HitResult.GetActor());
	
	if (!AbilityTag.IsValid()) return;
	
	// 현재 상호작용 중인 AbilityTag 추적 - 맥락을 받아온다
	CurrentInteractAbilityTag = AbilityTag;
	
	// 어빌리티 스펙 조회해서 Toggle 여부 캐싱
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->AbilityTags.HasTag(AbilityTag))
		{
			if (UHeistGameplayAbility* HGA = Cast<UHeistGameplayAbility>(Spec.Ability))
				bCurrentInteractIsToggle = HGA->IsToggleInteraction();
			break;
		}
	}
	
	FGameplayEventData Payload;
	Payload.Instigator = Pawn;
	Payload.Target     = HitResult.GetActor();
	// 상호작용 태그가 Ability Trigger 목록에 있는지 확인하고 그 Ability를 실행하도록 한다 - 즉 맥락대로 수행됨 
	// Gameplay Event를 통해 Tag를 던져 맥락기반으로 상호작용을 수행할 수 있습니다!
	ASC->HandleGameplayEvent(AbilityTag, &Payload);
}

void UHeistPlayerComponent::HandleInteractReleased()
{
	// 클라이언트에서 라인트레이스 수행하고, 결과만 서버에 던지도록 구현합니다.
	APawn* Pawn = GetPawn<APawn>();
	if (!IsValid(Pawn)) return;

	// 이녀석은 어빌리티 시스템을 간접 참조하기 위해 사용, 초기화할때 PawnExtensionComponent에 캐싱됨
	UHeistPawnExtensionComponent* PawnExtension = UHeistPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!IsValid(PawnExtension)) return;

	UHeistAbilitySystemComponent* ASC = PawnExtension->GetAbilitySystemComponent();
	if (!IsValid(ASC)) return;
	
	if (bCurrentInteractIsToggle)
	{
		bCurrentInteractIsToggle = false;
		CurrentInteractAbilityTag = FGameplayTag::EmptyTag;
		return;  // 토글 어빌리티는 Release 시 아무것도 안 함
	}
	
	// TODO: Release 시 상자 드롭하는 로직 - 가구현 각도를 어떻게 처리를 해야할걸요?
	if (ASC->HasMatchingGameplayTag(HeistFlagTags::Tag_Carrying))
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(HeistFlagTags::Tag_Carrying);
		ASC->CancelAbilities(&Tags);

		CurrentInteractAbilityTag = FGameplayTag::EmptyTag;
		return;
	}
	
	// 나머지 WhileInputActive(채널링 중) GA 취소 (HelpCuffed, Heal 등)
	if (CurrentInteractAbilityTag.IsValid())
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(CurrentInteractAbilityTag);
		// 여기서, HeistGameplayAbility::EndAbility가 bWasCancelled=true일 때 OnChannelingCancelled()를 
		// 호출하도록 이미 구현되어 있으므로, 별도 처리 없이 기존 명세대로 동작합니다.
		ASC->CancelAbilities(&Tags, nullptr, nullptr);
		CurrentInteractAbilityTag = FGameplayTag::EmptyTag;
	}

}
