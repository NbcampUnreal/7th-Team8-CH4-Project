#include "Core/HeistPlayerController.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "Core/HeistPlayerState.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "GameFramework/Pawn.h"
#include "EnhancedInputComponent.h"

AHeistPlayerController::AHeistPlayerController()
{
	bShowMouseCursor = true;
}

void AHeistPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsValid(SystemMenuInputAction)) return;

	UEnhancedInputComponent* EnhancedIC = CastChecked<UEnhancedInputComponent>(InputComponent);
	EnhancedIC->BindAction(SystemMenuInputAction, ETriggerEvent::Started, this, &ThisClass::Input_SystemMenu);
}

void AHeistPlayerController::Input_SystemMenu()
{
	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(HeistMessageTags::Message_SystemMenu_Toggle, FHeistSystemMenuToggleMessage{});
}

void AHeistPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdateCursorRotation();

	AHeistPlayerState* HeistPS = GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPS)) return;

	UHeistAbilitySystemComponent* ASC = HeistPS->GetHeistAbilitySystemComponent();
	if (!IsValid(ASC)) return;

	ASC->ProcessAbilityInput(DeltaTime, false);
}

void AHeistPlayerController::UpdateCursorRotation()
{
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn)) return;

	FVector RayOrigin;
	FVector RayDirection;
	if (!DeprojectMousePositionToWorld(RayOrigin, RayDirection)) return;

	if (FMath::IsNearlyZero(RayDirection.Z)) return;

	float GroundZ = ControlledPawn->GetActorLocation().Z;
	float T = (GroundZ - RayOrigin.Z) / RayDirection.Z;
	if (T < 0.0f) return;

	FVector CursorWorldPosition = RayOrigin + RayDirection * T;

	FVector LookDirection = CursorWorldPosition - ControlledPawn->GetActorLocation();
	LookDirection.Z = 0.0f;

	if (LookDirection.IsNearlyZero()) return;

	SetControlRotation(FRotator(0.0f, LookDirection.Rotation().Yaw, 0.0f));
}
