#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "HeistLobbyGameState.generated.h"

/**
 * 로비 맵 전용 GameState.
 * 접속 중인 플레이어 이름 목록을 복제하고
 * 변경 시 HeistMessageSubsystem으로 브로드캐스트한다.
 */
UCLASS()
class HEIST_API AHeistLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	void AddPlayer(const FString& PlayerName);
	void RemovePlayer(const FString& PlayerName);

	const TArray<FString>& GetLobbyPlayerNames() const { return LobbyPlayerNames; }

private:
	UPROPERTY(ReplicatedUsing = OnRep_LobbyPlayerNames)
	TArray<FString> LobbyPlayerNames;

	UFUNCTION()
	void OnRep_LobbyPlayerNames();

	void BroadcastPlayersChanged();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
