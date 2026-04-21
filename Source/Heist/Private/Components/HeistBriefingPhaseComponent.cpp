
#include "Components/HeistBriefingPhaseComponent.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "Components/HeistBriefingPlayerComponent.h"
#include "Core/HeistBriefingDrawingBoard.h"
#include "Core/HeistBriefingPointDataAsset.h"
#include "Core/HeistMatchGameState.h"
#include "Core/HeistPlayerState.h"
#include "GameplayEffect.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "Core/HeistMatchGameMode.h"
#include "Kismet/GameplayStatics.h"

void UHeistBriefingPhaseComponent::EnterBriefingPhase()
{
	ThiefSpawnSelections.Reset();
	PoliceObjectiveSelections.Reset();

	// 서버는 "팀 배정 -> 브리핑용 폰 스폰/재시작 -> 브리핑 컨텍스트 바인딩" 순서를 보장한다.
	// 클라이언트는 이 결과를 복제로 각기 다른 시점에 받으므로, 아래 순서를 기준으로 각 OnRep/훅에서 수렴한다.
	UE_LOG(LogTemp, Log, TEXT("[BriefingPhase] EnterBriefingPhase: begin server sequence"));
	AssignRandomRoles(); // 팀 부여
	RequestPlayersSpawnAtBriefingStart(); // 팀 확정 후 브리핑 시작 위치로 스폰/재시작
	ApplyBriefingStateToPlayers(); // Brief State 부여
	GatherThiefSpawnPoints(); // 도둑 스폰 포인트 수집
	GatherPoliceObjectivePoints(); // 경찰 물건 포인트 수집
	SpawnBriefingActors(); // 브리핑 엑터 생성
	BindPlayersToBriefingActors(); // 생성 이후 바인딩
	UE_LOG(LogTemp, Log, TEXT("[BriefingPhase] EnterBriefingPhase: server sequence completed"));
}

void UHeistBriefingPhaseComponent::LockSelections()
{
	FinalizeDefaultSelections();
}

void UHeistBriefingPhaseComponent::ExitBriefingPhase()
{
	RemoveBriefingStateFromPlayers();
	DrawingBoard = nullptr;
}

void UHeistBriefingPhaseComponent::TrySetThiefSpawnSelection(APlayerState* PlayerState, FName InKey)
{
	const AHeistMatchGameState* HeistGS = GetWorld() ? GetWorld()->GetGameState<AHeistMatchGameState>() : nullptr;
	if (!IsValid(HeistGS) || !HeistGS->IsBriefingPhase() || HeistGS->IsBriefingSelectionLocked()) return;
	if (!ThiefSpawnPointMap.Contains(InKey)) return;

	SetThiefSpawnSelection(PlayerState, InKey);
}

void UHeistBriefingPhaseComponent::TrySetPoliceObjectiveSelection(APlayerState* PlayerState, FName InKey)
{
	const AHeistMatchGameState* HeistGS = GetWorld() ? GetWorld()->GetGameState<AHeistMatchGameState>() : nullptr;
	if (!IsValid(HeistGS) || !HeistGS->IsBriefingPhase() || HeistGS->IsBriefingSelectionLocked()) return;
	if (!PoliceObjectivePointMap.Contains(InKey)) return;

	SetPoliceObjectiveSelection(PlayerState, InKey);
}

static TArray<FHeistBriefingSelectionCount> ComputeThiefCounts(
	const TMap<TObjectPtr<APlayerState>, FName>& Selections)
{
	TMap<FName, int32> CountMap;
	for (const auto& Pair : Selections)
		if (Pair.Value != NAME_None)
			CountMap.FindOrAdd(Pair.Value)++;

	TArray<FHeistBriefingSelectionCount> Result;
	for (const auto& Pair : CountMap)
	{
		FHeistBriefingSelectionCount Entry;
		Entry.Key   = Pair.Key;
		Entry.Count = Pair.Value;
		Result.Add(Entry);
	}
	return Result;
}

void UHeistBriefingPhaseComponent::PushThiefCountsTo(UHeistBriefingPlayerComponent* Target)
{
	if (IsValid(Target))
		Target->ClientReceiveThiefCounts(ComputeThiefCounts(ThiefSpawnSelections));
}

void UHeistBriefingPhaseComponent::SetThiefSpawnSelection(APlayerState* PlayerState, FName InKey)
{
	if (!IsValid(PlayerState))
	{
		return;
	}

	ThiefSpawnSelections.FindOrAdd(PlayerState) = InKey;

	// 모든 플레이어에게 갱신 카운트 push
	AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (IsValid(GS))
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			if (AHeistPlayerState* HPS = Cast<AHeistPlayerState>(PS))
				PushThiefCountsTo(HPS->GetBriefingPlayerComponent());
		}
	}
}

void UHeistBriefingPhaseComponent::SetPoliceObjectiveSelection(APlayerState* PlayerState, FName InKey)
{
	if (!IsValid(PlayerState))
	{
		return;
	}

	PoliceObjectiveSelections.FindOrAdd(PlayerState) = InKey;

	if (AHeistPlayerState* HeistPlayerState = Cast<AHeistPlayerState>(PlayerState))
	{
		if (UHeistBriefingPlayerComponent* BriefingPlayerComponent = HeistPlayerState->GetBriefingPlayerComponent())
		{
			BriefingPlayerComponent->ClientReceivePoliceObjectiveSelection(InKey);
		}
	}
}

FName UHeistBriefingPhaseComponent::GetThiefSpawnSelection(const APlayerState* PlayerState) const
{
	if (const FName* Found = ThiefSpawnSelections.Find(PlayerState))
	{
		return *Found;
	}

	return NAME_None;
}

FName UHeistBriefingPhaseComponent::GetPoliceObjectiveSelection(const APlayerState* PlayerState) const
{
	if (const FName* Found = PoliceObjectiveSelections.Find(PlayerState))
	{
		return *Found;
	}

	return NAME_None;
}

void UHeistBriefingPhaseComponent::AssignRandomRoles()
{
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!IsValid(GameState)) return;

	TArray<AHeistPlayerState*> Players;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(PS))
		{
			Players.Add(HeistPS);
		}
	}

	if (Players.IsEmpty()) return;

	// Fisher-Yates 셔플
	for (int32 i = Players.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Players.Swap(i, j);
	}

	// 첫 번째 플레이어 = 경찰(술래), 나머지 = 도둑
	Players[0]->SetAssignedTeam(EHeistTeam::Police);
	UE_LOG(LogTemp, Log, TEXT("[BriefingPhase] AssignRandomRoles: %s -> Police"),
		*Players[0]->GetName());
	for (int32 i = 1; i < Players.Num(); ++i)
	{
		Players[i]->SetAssignedTeam(EHeistTeam::Thief);
		UE_LOG(LogTemp, Log, TEXT("[BriefingPhase] AssignRandomRoles: %s -> Thief"),
			*Players[i]->GetName());
	}

	// 드로잉 테스트 코드
	// for (AHeistPlayerState* Player : Players)
	// {
	// 	if (!IsValid(Player))
	// 	{
	// 		continue;
	// 	}
	//
	// 	Player->SetAssignedTeam(EHeistTeam::Thief);
	// }
}

void UHeistBriefingPhaseComponent::RequestPlayersSpawnAtBriefingStart()
{
	AHeistMatchGameMode* HeistGM = GetOwner<AHeistMatchGameMode>();
	if (!IsValid(HeistGM))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BriefingPhase] RequestPlayersSpawnAtBriefingStart: missing HeistMatchGameMode owner"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[BriefingPhase] RequestPlayersSpawnAtBriefingStart: delegating spawn to GameMode"));
	HeistGM->SpawnAllPlayersAtBriefingStart();
}

void UHeistBriefingPhaseComponent::SpawnBriefingActors()
{
	if (!IsValid(DrawingBoard))
	{
		UWorld* World = GetWorld();
		if (!IsValid(World))
		{
			return;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		DrawingBoard = World->SpawnActor<AHeistBriefingDrawingBoard>(
			DrawingBoardClass,
			FTransform::Identity,
			SpawnParams);

		UE_LOG(LogTemp, Log, TEXT("[BriefingPhase] SpawnBriefingActors: DrawingBoard=%s"),
			IsValid(DrawingBoard) ? *DrawingBoard->GetName() : TEXT("None"));
	}
}

void UHeistBriefingPhaseComponent::BindPlayersToBriefingActors()
{
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!IsValid(GameState) || !IsValid(DrawingBoard))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BriefingPhase] BindPlayersToBriefingActors: GameState=%d DrawingBoard=%d"),
			IsValid(GameState) ? 1 : 0,
			IsValid(DrawingBoard) ? 1 : 0);
		return;
	}

	// 여기서 서버는 플레이어별 브리핑 컨텍스트를 발행한다.
	// 클라이언트는 DrawBoard/AssignedTeam/Controller/Pawn 복제가 제각각 도착하므로,
	// OnRep_DrawingBoard / OnRep_AssignedTeam / OnRep_PlayerState / AcknowledgePossession 훅에서 다시 수렴한다.
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AHeistPlayerState* HeistPlayerState = Cast<AHeistPlayerState>(PlayerState);
		if (!IsValid(HeistPlayerState))
		{
			continue;
		}

		UHeistBriefingPlayerComponent* BriefingPlayerComponent = HeistPlayerState->GetBriefingPlayerComponent();
		if (!IsValid(BriefingPlayerComponent))
		{
			continue;
		}

		const EHeistBriefingViewMode ViewMode =
			HeistPlayerState->IsPolice() ? EHeistBriefingViewMode::Police : EHeistBriefingViewMode::Thief;

		BriefingPlayerComponent->InitializeBriefingContext(DrawingBoard, ViewMode);
		BriefingPlayerComponent->SetBriefingPhase(this);
		UE_LOG(LogTemp, Log, TEXT("[BriefingPhase] BindPlayersToBriefingActors: PS=%s Team=%d ViewMode=%d"),
			*HeistPlayerState->GetName(),
			static_cast<int32>(HeistPlayerState->GetAssignedTeam()),
			static_cast<int32>(ViewMode));
	}
}

void UHeistBriefingPhaseComponent::FinalizeDefaultSelections()
{
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!IsValid(GameState)) return;

	for (APlayerState* PS : GameState->PlayerArray)
	{
		AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(PS);
		if (!IsValid(HeistPS)) continue;

		// 미선택 도둑에게 첫 번째 후보 기본 부여
		if (HeistPS->IsThief() && GetThiefSpawnSelection(HeistPS) == NAME_None)
		{
			if (!ThiefSpawnPointMap.IsEmpty())
			{
				SetThiefSpawnSelection(HeistPS, ThiefSpawnPointMap.begin().Key());
			}
		}

		// 미선택 경찰에게 첫 번째 후보 기본 부여
		if (HeistPS->IsPolice() && GetPoliceObjectiveSelection(HeistPS) == NAME_None)
		{
			if (!PoliceObjectivePointMap.IsEmpty())
			{
				SetPoliceObjectiveSelection(HeistPS, PoliceObjectivePointMap.begin().Key());
			}
		}
	}
}

void UHeistBriefingPhaseComponent::ApplyBriefingStateToPlayers()
{
	BriefingStateEffectHandles.Reset();

	if (!IsValid(BriefingStateEffectClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("BriefingPhaseComponent: BriefingStateEffectClass가 설정되지 않았습니다."));
		return;
	}

	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!IsValid(GameState))
	{
		return;
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(PlayerState);
		if (!IsValid(HeistPS))
		{
			continue;
		}

		UHeistAbilitySystemComponent* ASC = HeistPS->GetHeistAbilitySystemComponent();
		if (!IsValid(ASC))
		{
			continue;
		}

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(BriefingStateEffectClass, 1.0f, Context);
		if (!Spec.IsValid() || !Spec.Data.IsValid())
		{
			continue;
		}

		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (Handle.IsValid())
		{
			BriefingStateEffectHandles.Add(HeistPS, Handle);
		}
	}
}

void UHeistBriefingPhaseComponent::RemoveBriefingStateFromPlayers()
{
	for (TPair<TWeakObjectPtr<APlayerState>, FActiveGameplayEffectHandle>& Pair : BriefingStateEffectHandles)
	{
		APlayerState* PlayerState = Pair.Key.Get();
		AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(PlayerState);
		if (!IsValid(HeistPS))
		{
			continue;
		}

		UHeistAbilitySystemComponent* ASC = HeistPS->GetHeistAbilitySystemComponent();
		if (!IsValid(ASC) || !Pair.Value.IsValid())
		{
			continue;
		}

		ASC->RemoveActiveGameplayEffect(Pair.Value);
	}

	BriefingStateEffectHandles.Reset();
}

void UHeistBriefingPhaseComponent::GatherThiefSpawnPoints()
{
	ThiefSpawnPointMap.Reset();

	if (!IsValid(BriefingPointData))
	{
		UE_LOG(LogTemp, Warning, TEXT("BriefingPhaseComponent: BriefingPointData가 설정되지 않았습니다."));
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	for (const FHeistSpawnPointData& Def : BriefingPointData->Points)
	{
		if (Def.PointType != EHeistBriefingPointType::ThiefInsertion) continue;
		if (Def.WorldTag == NAME_None) continue;

		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(World, Def.WorldTag, Found);
		if (!Found.IsEmpty())
		{
			ThiefSpawnPointMap.Add(Def.Key, Found[0]->GetActorLocation());
		}
	}

	if (ThiefSpawnPointMap.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BriefingPhaseComponent: 도둑 스폰 포인트를 하나도 수집하지 못했습니다."));
	}
}

void UHeistBriefingPhaseComponent::GatherPoliceObjectivePoints()
{
	PoliceObjectivePointMap.Reset();

	if (!IsValid(BriefingPointData))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	for (const FHeistSpawnPointData& Def : BriefingPointData->Points)
	{
		if (Def.PointType != EHeistBriefingPointType::PoliceObjective) continue;
		if (Def.WorldTag == NAME_None) continue;

		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(World, Def.WorldTag, Found);
		if (!Found.IsEmpty())
		{
			PoliceObjectivePointMap.Add(Def.Key, Found[0]->GetActorLocation());
		}
	}

	if (PoliceObjectivePointMap.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BriefingPhaseComponent: 경찰 목표 포인트를 하나도 수집하지 못했습니다."));
	}
}
