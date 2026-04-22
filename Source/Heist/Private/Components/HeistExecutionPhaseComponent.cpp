
#include "Components/HeistExecutionPhaseComponent.h"

#include "Components/HeistArrestVictoryComponent.h"
#include "Components/HeistBriefingPhaseComponent.h"
#include "Core/HeistMatchTypes.h"
#include "Core/HeistMatchGameMode.h"
#include "Core/HeistMatchGameState.h"
#include "Core/HeistPlayerController.h"
#include "Core/HeistPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

void UHeistExecutionPhaseComponent::EnterExecutionPhase()
{
	ApplyThiefInsertionSpawns();
	SpawnPoliceObjective();
	RestoreVoiceToPawnRoot();
	ActivateExecutionGameplay();

	AHeistMatchGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AHeistMatchGameMode>() : nullptr;
	if (!IsValid(GM)) return;

	if (UHeistArrestVictoryComponent* ArrestComp = GM->FindComponentByClass<UHeistArrestVictoryComponent>())
	{
		ArrestComp->RegisterAllThieves();
	}
}

void UHeistExecutionPhaseComponent::ApplyThiefInsertionSpawns()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	AHeistMatchGameMode* GM = World->GetAuthGameMode<AHeistMatchGameMode>();
	if (!IsValid(GM)) return;

	UHeistBriefingPhaseComponent* BriefingPhase = GM->FindComponentByClass<UHeistBriefingPhaseComponent>();
	if (!IsValid(BriefingPhase)) return;

	const TMap<FName, FVector>& SpawnMap = BriefingPhase->GetThiefSpawnPointMap();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!IsValid(PlayerController))
		{
			continue;
		}

		AHeistPlayerState* HeistPS = PlayerController->GetPlayerState<AHeistPlayerState>();
		if (!IsValid(HeistPS) || !HeistPS->IsThief()) continue;

		APawn* Pawn = PlayerController->GetPawn();
		if (!IsValid(Pawn)) continue;

		const FName SelectionKey = BriefingPhase->GetThiefSpawnSelection(HeistPS);
		if (SelectionKey == NAME_None) continue;

		const FVector* SpawnLocation = SpawnMap.Find(SelectionKey);
		if (!SpawnLocation) continue;

		const FRotator SpawnRotation = Pawn->GetActorRotation();
		FVector ChosenSpawnLocation = *SpawnLocation;

		if (UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation NavLocation;
			constexpr float NavSearchRadius = 300.f;

			if (NavigationSystem->GetRandomReachablePointInRadius(*SpawnLocation, NavSearchRadius, NavLocation))
			{
				ChosenSpawnLocation = NavLocation.Location;
			}
		}

		Pawn->TeleportTo(ChosenSpawnLocation, SpawnRotation, false, true);
	}
}

void UHeistExecutionPhaseComponent::SpawnPoliceObjective()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	AGameStateBase* GameState = World->GetGameState();
	if (!IsValid(GameState)) return;

	AHeistMatchGameState* HeistGS = Cast<AHeistMatchGameState>(GameState);
	if (!IsValid(HeistGS)) return;

	AHeistMatchGameMode* GM = World->GetAuthGameMode<AHeistMatchGameMode>();
	if (!IsValid(GM)) return;

	UHeistBriefingPhaseComponent* BriefingPhase = GM->FindComponentByClass<UHeistBriefingPhaseComponent>();
	if (!IsValid(BriefingPhase)) return;

	// 경찰 PlayerState에서 선택 키 읽기
	FName SelectedKey = NAME_None;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		const AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(PS);
		if (IsValid(HeistPS) && HeistPS->IsPolice())
		{
			SelectedKey = BriefingPhase->GetPoliceObjectiveSelection(HeistPS);
			break;
		}
	}

	if (SelectedKey == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecutionPhaseComponent: 경찰 목표 선택이 없습니다."));
		return;
	}

	HeistGS->SetPoliceObjectiveDisplayName(BriefingPhase->GetPoliceObjectiveDisplayName(SelectedKey));

	if (!IsValid(PoliceObjectiveClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecutionPhaseComponent: PoliceObjectiveClass가 설정되지 않았습니다. GameMode BP에서 할당하세요."));
		return;
	}

	const FVector* SpawnLocation = BriefingPhase->GetPoliceObjectivePointMap().Find(SelectedKey);
	if (!SpawnLocation)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecutionPhaseComponent: 키 '%s'에 해당하는 목표 위치가 없습니다."), *SelectedKey.ToString());
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	World->SpawnActor<AActor>(PoliceObjectiveClass, *SpawnLocation, FRotator::ZeroRotator, SpawnParams);
}

void UHeistExecutionPhaseComponent::RestoreVoiceToPawnRoot()
{
	// 브리핑과 실행 모두 현재는 Pawn Root 기반 보이스를 유지한다.
	// VoiceAnchor 분리 연출은 보류 상태이므로 no-op로 둔다.
}

void UHeistExecutionPhaseComponent::ActivateExecutionGameplay()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	AHeistMatchGameMode* GM = World->GetAuthGameMode<AHeistMatchGameMode>();
	if (!IsValid(GM))
	{
		return;
	}

	if (UHeistBriefingPhaseComponent* BriefingPhase = GM->FindComponentByClass<UHeistBriefingPhaseComponent>())
	{
		BriefingPhase->ExitBriefingPhase();
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		AHeistPlayerController* HeistPC = Cast<AHeistPlayerController>(It->Get());
		if (!IsValid(HeistPC))
		{
			continue;
		}

		HeistPC->ClientEndBriefingPresentation();
	}
}
