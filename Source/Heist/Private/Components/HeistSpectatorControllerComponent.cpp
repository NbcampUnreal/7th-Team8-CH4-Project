#include "Components/HeistSpectatorControllerComponent.h"

#include "Components/InputComponent.h"
#include "Core/HeistPlayerController.h"
#include "Core/HeistPlayerState.h"
#include "EngineUtils.h"
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
	CurrentViewTarget.Reset();
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
	}
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
	TArray<AActor*> Candidates;
	GatherSpectateCandidates(Candidates);
	return Candidates.IsEmpty() ? nullptr : Candidates[0];
}

void UHeistSpectatorControllerComponent::GatherSpectateCandidates(TArray<AActor*>& OutCandidates) const
{
	OutCandidates.Reset();

	AHeistPlayerController* HeistPC = GetHeistPlayerController();
	UWorld* World = GetWorld();
	if (!IsValid(HeistPC) || !IsValid(World)) return;

	const APawn* LocalPawn = HeistPC->GetPawn();
	const AHeistPlayerState* LocalPlayerState = HeistPC->GetPlayerState<AHeistPlayerState>();

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* CandidatePawn = *It;
		if (!IsValid(CandidatePawn) || CandidatePawn == LocalPawn) continue;

		AHeistPlayerState* CandidatePlayerState = CandidatePawn ? CandidatePawn->GetPlayerState<AHeistPlayerState>() : nullptr;
		if (!IsValid(CandidatePlayerState) || CandidatePlayerState == LocalPlayerState) continue;

		const EHeistTeam CandidateTeam = CandidatePlayerState->GetAssignedTeam();
		if (CandidateTeam == EHeistTeam::None || CandidateTeam == EHeistTeam::Spector) continue;

		OutCandidates.Add(CandidatePawn);
	}
}

AActor* UHeistSpectatorControllerComponent::FindSpectateTargetFromCurrent(int32 Direction) const
{
	AHeistPlayerController* HeistPC = GetHeistPlayerController();
	if (!IsValid(HeistPC)) return nullptr;

	TArray<AActor*> Candidates;
	GatherSpectateCandidates(Candidates);
	if (Candidates.IsEmpty()) return nullptr;

	const int32 CurrentIndex = Candidates.IndexOfByKey(HeistPC->GetViewTarget());
	if (CurrentIndex == INDEX_NONE) return Candidates[0];

	const int32 NextIndex = (CurrentIndex + Direction + Candidates.Num()) % Candidates.Num();
	return Candidates[NextIndex];
}

bool UHeistSpectatorControllerComponent::IsCurrentViewTargetValid() const
{
	AHeistPlayerController* HeistPC = GetHeistPlayerController();
	if (!IsValid(HeistPC)) return false;

	AActor* ActualViewTarget = HeistPC->GetViewTarget();
	if (!IsValid(ActualViewTarget)) return false;

	TArray<AActor*> Candidates;
	GatherSpectateCandidates(Candidates);
	return Candidates.Contains(ActualViewTarget);
}
