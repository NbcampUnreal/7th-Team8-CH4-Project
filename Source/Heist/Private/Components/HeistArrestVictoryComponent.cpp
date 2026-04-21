#include "Components/HeistArrestVictoryComponent.h"

#include "Character/ThiefCharacter.h"
#include "Core/HeistMatchGameMode.h"
#include "Core/HeistMatchTypes.h"
#include "Core/HeistPlayerController.h"
#include "Core/HeistPlayerState.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpectatorPawn.h"

void UHeistArrestVictoryComponent::RegisterAllThieves()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	RegisteredThieves.Reset();
	ArrestedThieves.Reset();

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	AHeistMatchGameMode* GM = World->GetAuthGameMode<AHeistMatchGameMode>();
	AGameStateBase* GameState = World->GetGameState();
	if (!IsValid(GM) || !IsValid(GameState)) return;

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(PlayerState);
		if (!IsValid(HeistPS) || !HeistPS->IsThief()) continue;

		RegisteredThieves.Add(HeistPS);
	}
}

void UHeistArrestVictoryComponent::NotifyThiefArrested(AThiefCharacter* ArrestedThief)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!IsValid(ArrestedThief)) return;

	AHeistPlayerState* HeistPS = ArrestedThief->GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPS)) return;

	const TWeakObjectPtr<AHeistPlayerState> PlayerStatePtr = HeistPS;
	if (ArrestedThieves.Contains(PlayerStatePtr)) return;

	ArrestedThieves.Add(PlayerStatePtr);
	TransitionToSpectator(ArrestedThief);
	CheckAllArrested();
}

void UHeistArrestVictoryComponent::NotifyThiefDisconnected(AHeistPlayerState* DisconnectedPS)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!IsValid(DisconnectedPS)) return;

	const TWeakObjectPtr<AHeistPlayerState> PlayerStatePtr = DisconnectedPS;
	if (ArrestedThieves.Contains(PlayerStatePtr)) return;

	ArrestedThieves.Add(PlayerStatePtr);
	CheckAllArrested();
}

void UHeistArrestVictoryComponent::TransitionToSpectator(AThiefCharacter* Thief)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	AHeistPlayerController* HeistPC = Cast<AHeistPlayerController>(Thief->GetController());
	if (!IsValid(HeistPC)) return;

	AHeistPlayerState* HeistPS = HeistPC->GetPlayerState<AHeistPlayerState>();
	if (IsValid(HeistPS))
	{
		HeistPS->SetAssignedTeam(EHeistTeam::Spector);
	}

	const FVector SpawnLocation = Thief->GetActorLocation();
	const FRotator SpawnRotation = Thief->GetActorRotation();

	HeistPC->UnPossess();
	Thief->Destroy();

	AGameModeBase* GM = GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr;
	if (!IsValid(GM) || !IsValid(GM->SpectatorClass)) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = HeistPC;

	ASpectatorPawn* SpectatorPawn = GetWorld()->SpawnActor<ASpectatorPawn>(
		GM->SpectatorClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams);
	if (!IsValid(SpectatorPawn)) return;

	HeistPC->Possess(SpectatorPawn);
	HeistPC->ClientNotifyArrested();
}

void UHeistArrestVictoryComponent::CheckAllArrested()
{
	// 도둑 0명 상태는 비정상 Execution 진입으로 간주하고 승리를 선언하지 않는다.
	if (RegisteredThieves.IsEmpty()) return;

	for (const TWeakObjectPtr<AHeistPlayerState>& HeistPS : RegisteredThieves)
	{
		if (!ArrestedThieves.Contains(HeistPS)) return;
	}

	OnPoliceVictory.Broadcast();
}
