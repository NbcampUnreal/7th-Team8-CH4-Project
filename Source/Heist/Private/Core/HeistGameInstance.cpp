#include "HeistGameInstance.h"

#include "MultiplayerSessionTypes.h"

UHeistGameInstance::UHeistGameInstance()
{
	LobbyPath = TEXT("/Game/Maps/Lobby");
	bUseDedicatedServer = false;
	bDestroySessionOnStart = false;
}

void UHeistGameInstance::RequestDedicatedServerAddress(EMultiplayerTravelPurpose Purpose)
{
	// TODO: 데디케이티드 서버 인프라 확정 후 매치메이킹 서비스에서 주소를 가져오도록 구현
	// Purpose == EMultiplayerTravelPurpose::Lobby → 로비 서버 주소 요청
	// Purpose == EMultiplayerTravelPurpose::Game  → 게임 서버 주소 요청
	// 준비되면 OnDedicatedServerAddressReceived(Address, Purpose) 호출

	// Listen-server 모드(bUseDedicatedServer=false)에서는 이 함수가 호출되지 않는다.
	// 이 stub은 bUseDedicatedServer=true로 전환 시를 대비한 오버라이드 포인트다.
	Super::RequestDedicatedServerAddress(Purpose);
}
