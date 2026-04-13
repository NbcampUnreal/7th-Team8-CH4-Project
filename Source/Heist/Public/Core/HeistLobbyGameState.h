#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "HeistLobbyGameState.generated.h"

/**
 * 로비 맵 전용 GameState.
 * 초대 코드를 복제하고 변경 시 HeistMessageSubsystem으로 브로드캐스트한다.
 * 플레이어 목록은 GameStateBase::PlayerArray를 통해 직접 접근한다.
 */
UCLASS()
class HEIST_API AHeistLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	void SetInviteCode(const FString& NewInviteCode);

	const FString& GetInviteCode() const { return InviteCode; }

	bool AreAllPlayersReady() const;

private:
	UPROPERTY(ReplicatedUsing = OnRep_InviteCode)
	FString InviteCode;

	UFUNCTION()
	void OnRep_InviteCode();

	void BroadcastPlayersChanged();
	void BroadcastInviteCodeChanged();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
