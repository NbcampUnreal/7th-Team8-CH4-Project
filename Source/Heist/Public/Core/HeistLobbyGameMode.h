#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HeistLobbyGameMode.generated.h"

/**
 * 로비 맵 전용 GameMode.
 * 플레이어 입퇴장을 감지하고, 호스트의 요청으로 게임 맵으로 이동한다.
 */
UCLASS()
class HEIST_API AHeistLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHeistLobbyGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/**
	 * 게임 시작 요청. 호스트만 유효하게 처리된다.
	 * 호스트 판단: GameState->PlayerArray[0]의 PlayerController.
	 * 최소 인원(MinPlayersToStart) 미충족 시 무시.
	 */
	void RequestStartGame(APlayerController* Requester);

private:
	// 게임 맵 경로. Blueprint Class Defaults에서 설정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	FString GameMapPath;

	// 게임 시작에 필요한 최소 플레이어 수.
	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	int32 MinPlayersToStart;

	bool IsHostController(APlayerController* PlayerController) const;
};
