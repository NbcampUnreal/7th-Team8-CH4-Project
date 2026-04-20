#include "Voice/HeistVoiceSubsystem.h"

#include "Core/HeistPlayerController.h"
#include "Core/HeistMatchGameState.h"
#include "Voice/HeistVoipTravelUtils.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/VoiceConfig.h"

void UHeistVoiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	WorldBeginTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(
		this, &UHeistVoiceSubsystem::HandleWorldBeginTearDown);

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UHeistVoiceSubsystem::HandlePostLoadMap);
}

void UHeistVoiceSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldBeginTearDown.Remove(WorldBeginTearDownHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

	Super::Deinitialize();
}

void UHeistVoiceSubsystem::BeginTravelShutdown(UWorld* World)
{
	if (bTravelShutdownInProgress) return;
	bTravelShutdownInProgress = true;

	HeistVoipTravelUtils::ShutdownVoiceForTravel(World, true);

	// 모든 플레이어의 voice talker를 초기화해 이전 연결 상태를 정리
	if (IsValid(World))
	{
		if (AGameStateBase* GS = World->GetGameState())
		{
			for (APlayerState* PS : GS->PlayerArray)
			{
				if (IsValid(PS))
				{
					UVOIPStatics::ResetPlayerVoiceTalker(PS);
				}
			}
		}
	}

	HeistVoipTravelUtils::UnregisterTransientVoipComps();
}

void UHeistVoiceSubsystem::HandleWorldBeginTearDown(UWorld* World)
{
	// UnregisterRemoteTalker 처리 이후 생성된 transient 컴포넌트까지 처리하는 최후 정리 지점
	HeistVoipTravelUtils::UnregisterTransientVoipComps();
}

void UHeistVoiceSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	// 트래블 완료 — 복구는 Pawn/PS 준비 확인 후 수행
	bPostTravelRecoveryPending = true;
}

void UHeistVoiceSubsystem::RequestVoiceRefresh(AHeistPlayerController* PC)
{
	RefreshVoiceState(PC);
}

void UHeistVoiceSubsystem::OnPawnPossessed(AHeistPlayerController* PC, APawn* NewPawn)
{
	TryRecoverVoiceForPlayer(PC);
}

void UHeistVoiceSubsystem::OnPlayerStateReady(AHeistPlayerController* PC)
{
	TryRecoverVoiceForPlayer(PC);
}

void UHeistVoiceSubsystem::TryRecoverVoiceForPlayer(AHeistPlayerController* PC)
{
	if (!IsValid(PC) || !PC->IsLocalController()) return;

	if (!bPostTravelRecoveryPending)
	{
		// 일반 경로(트래블 외) — 그냥 voice 상태 재평가
		RefreshVoiceState(PC);
		return;
	}

	// Pawn과 PlayerState가 모두 준비됐을 때만 복구 진행
	UWorld* World = PC->GetWorld();
	if (!IsValid(World)) return;
	if (World->GetMapName().Contains(TEXT("Transition"), ESearchCase::IgnoreCase)) return;
	if (!IsValid(PC->GetPawn())) return;
	if (!IsValid(PC->GetPlayerState<APlayerState>())) return;

	HeistVoipTravelUtils::RestoreRemoteTalkers(World, PC);
	bPostTravelRecoveryPending = false;
	bTravelShutdownInProgress = false;
	RefreshVoiceState(PC);
}

bool UHeistVoiceSubsystem::ShouldEnableVoice(AHeistPlayerController* PC) const
{
	if (!IsValid(PC) || !PC->IsLocalController()) return false;
	if (!IsValid(PC->GetPawn())) return false;
	if (bTravelShutdownInProgress) return false;

	const UWorld* World = PC->GetWorld();
	if (!IsValid(World)) return false;

	const AHeistMatchGameState* MatchGS = World->GetGameState<AHeistMatchGameState>();
	// MatchGameState 없으면 로비 월드 — 항상 허용
	if (!IsValid(MatchGS)) return true;

	return MatchGS->IsBriefingPhase() || MatchGS->IsExecutionPhase();
}

void UHeistVoiceSubsystem::RefreshVoiceState(AHeistPlayerController* PC)
{
	if (!IsValid(PC) || !PC->IsLocalController()) return;
	ApplyDesiredVoiceCaptureState(PC, ShouldEnableVoice(PC));
}

void UHeistVoiceSubsystem::ApplyDesiredVoiceCaptureState(AHeistPlayerController* PC, bool bShouldCapture)
{
	if (!IsValid(PC)) return;

	if (bShouldCapture)
	{
		PC->StartVoiceCapture();
	}
	else
	{
		PC->StopVoiceCapture();
	}
}
