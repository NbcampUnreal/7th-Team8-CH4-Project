#include "Components/HeistSpectatorControllerComponent.h"

#include "Components/InputComponent.h"
#include "Core/HeistPlayerController.h"
#include "Core/HeistPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"

UHeistSpectatorControllerComponent::UHeistSpectatorControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UHeistSpectatorControllerComponent::BindInput(UInputComponent* InputComponent)
{
	if (bInputBound || !IsValid(InputComponent)) return;

	AHeistPlayerController* HeistPC = GetHeistPlayerController();
	if (!IsValid(HeistPC) || !HeistPC->IsLocalController()) return;

	InputComponent->BindKey(PreviousSpectateKey, IE_Pressed, this, &ThisClass::SpectatePreviousTarget);
	InputComponent->BindKey(NextSpectateKey, IE_Pressed, this, &ThisClass::SpectateNextTarget);
	bInputBound = true;
}

void UHeistSpectatorControllerComponent::EnterArrestSpectating()
{
	AHeistPlayerController* HeistPC = GetHeistPlayerController();
	if (!IsValid(HeistPC) || !HeistPC->IsLocalController()) return;

	bArrestSpectatingActive = true;
	SetComponentTickEnabled(true);

	AActor* InitialTarget = FindFirstSpectateTarget();
	if (!IsValid(InitialTarget)) return;

	ApplyViewTarget(InitialTarget);
}

void UHeistSpectatorControllerComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bArrestSpectatingActive) return;
	if (IsCurrentViewTargetValid()) return;

	AActor* Target = FindFirstSpectateTarget();
	if (IsValid(Target))
	{
		ApplyViewTarget(Target);
		return;
	}

	AHeistPlayerController* HeistPC = GetHeistPlayerController();
	if (!IsValid(HeistPC) || !IsValid(HeistPC->GetPawn())) return;

	ApplyViewTarget(HeistPC->GetPawn());
}

void UHeistSpectatorControllerComponent::SpectateNextTarget()
{
	SpectateInDirection(1);
}

void UHeistSpectatorControllerComponent::SpectatePreviousTarget()
{
	SpectateInDirection(-1);
}

void UHeistSpectatorControllerComponent::SpectateInDirection(int32 Direction)
{
	AHeistPlayerController* HeistPC = GetHeistPlayerController();
	if (!bArrestSpectatingActive || !IsValid(HeistPC) || !HeistPC->IsLocalController()) return;

	AActor* Target = FindSpectateTargetFromCurrent(Direction);
	if (!IsValid(Target)) return;

	ApplyViewTarget(Target);
}

void UHeistSpectatorControllerComponent::ApplyViewTarget(AActor* Target)
{
	AHeistPlayerController* HeistPC = GetHeistPlayerController();
	if (!IsValid(HeistPC) || !IsValid(Target)) return;

	HeistPC->SetViewTargetWithBlend(Target, 0.35f);
	CurrentViewTarget = Target;
}

AHeistPlayerController* UHeistSpectatorControllerComponent::GetHeistPlayerController() const
{
	return GetOwner<AHeistPlayerController>();
}

AActor* UHeistSpectatorControllerComponent::FindFirstSpectateTarget() const
{
	AHeistPlayerController* HeistPC = GetHeistPlayerController();
	UWorld* World = GetWorld();
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!IsValid(HeistPC) || !IsValid(GameState)) return nullptr;

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(PlayerState);
		if (!IsSpectateCandidate(HeistPS)) continue;

		const APlayerController* TargetPC = Cast<APlayerController>(HeistPS->GetOwner());
		if (!IsValid(TargetPC) || !IsValid(TargetPC->GetPawn())) continue;

		return TargetPC->GetPawn();
	}

	return nullptr;
}

AActor* UHeistSpectatorControllerComponent::FindSpectateTargetFromCurrent(int32 Direction) const
{
	AHeistPlayerController* HeistPC = GetHeistPlayerController();
	UWorld* World = GetWorld();
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!IsValid(HeistPC) || !IsValid(GameState)) return nullptr;

	TArray<AActor*> Candidates;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(PlayerState);
		if (!IsSpectateCandidate(HeistPS)) continue;

		const APlayerController* TargetPC = Cast<APlayerController>(HeistPS->GetOwner());
		if (!IsValid(TargetPC) || !IsValid(TargetPC->GetPawn())) continue;

		Candidates.Add(TargetPC->GetPawn());
	}

	if (Candidates.IsEmpty()) return nullptr;

	const int32 CurrentIndex = Candidates.IndexOfByKey(CurrentViewTarget.Get());
	if (CurrentIndex == INDEX_NONE) return Candidates[0];

	const int32 NextIndex = (CurrentIndex + Direction + Candidates.Num()) % Candidates.Num();
	return Candidates[NextIndex];
}

bool UHeistSpectatorControllerComponent::IsSpectateCandidate(const AHeistPlayerState* HeistPS) const
{
	AHeistPlayerController* HeistPC = GetHeistPlayerController();
	if (!IsValid(HeistPC) || !IsValid(HeistPS)) return false;
	if (HeistPS == HeistPC->GetPlayerState<AHeistPlayerState>()) return false;
	if (HeistPS->GetAssignedTeam() == EHeistTeam::None || HeistPS->GetAssignedTeam() == EHeistTeam::Spector) return false;

	const APlayerController* TargetPC = Cast<APlayerController>(HeistPS->GetOwner());
	return IsValid(TargetPC) && IsValid(TargetPC->GetPawn());
}

bool UHeistSpectatorControllerComponent::IsCurrentViewTargetValid() const
{
	if (!CurrentViewTarget.IsValid()) return false;

	AHeistPlayerController* HeistPC = GetHeistPlayerController();
	UWorld* World = GetWorld();
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!IsValid(HeistPC) || !IsValid(GameState)) return false;

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(PlayerState);
		if (!IsSpectateCandidate(HeistPS)) continue;

		const APlayerController* TargetPC = Cast<APlayerController>(HeistPS->GetOwner());
		if (!IsValid(TargetPC) || !IsValid(TargetPC->GetPawn())) continue;
		if (TargetPC->GetPawn() != CurrentViewTarget.Get()) continue;

		return true;
	}

	return false;
}
