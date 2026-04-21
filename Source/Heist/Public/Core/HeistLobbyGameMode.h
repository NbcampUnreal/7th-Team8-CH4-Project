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

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/**
	 * 게임 시작 요청. 호스트만 유효하게 처리된다.
	 * 호스트 판단: GameState->PlayerArray[0]의 PlayerController.
	 * 최소 인원(MinPlayersToStart) 미충족 시 무시.
	 */
	void RequestStartGame(APlayerController* Requester);
	void RequestTogglePreviewCharacter(APlayerController* Requester);
	void NotifyPlayerReadyForMatchTravel(APlayerController* PlayerController);

private:
	void StartMatchTravel();
	void HandleMatchTravelReadyTimeout();
	int32 CountExpectedPlayersForMatchTravel() const;
	int32 CountReadyPlayersForMatchTravel() const;

	// 로비 프리뷰용 캐릭터 클래스. Blueprint Class Defaults에서 설정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Lobby|Preview")
	TSubclassOf<APawn> LobbyThiefCharacterClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lobby|Preview")
	TSubclassOf<APawn> LobbyPoliceCharacterClass;

	// 게임 맵 경로. Blueprint Class Defaults에서 설정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	FString GameMapPath;

	// 게임 시작에 필요한 최소 플레이어 수.
	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	int32 MinPlayersToStart;

	// 로비 시작 버튼 연타와 travel 중복 진입을 막는다.
	bool bStartGameRequested = false;

	// travel 준비 대상/완료 집계를 PlayerState 기준으로 고정해, 잔존 PlayerController에 흔들리지 않게 한다.
	TSet<TWeakObjectPtr<APlayerState>> ExpectedPlayersForMatchTravel;
	TSet<TWeakObjectPtr<APlayerState>> PlayersReadyForMatchTravel;

	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	float MatchTravelReadyTimeoutSeconds = 5.0f;

	FTimerHandle MatchTravelReadyTimeoutHandle;

	bool IsHostController(APlayerController* PlayerController) const;
};
