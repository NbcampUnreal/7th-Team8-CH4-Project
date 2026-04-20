#pragma once

class APlayerController;
class UWorld;

namespace HeistVoipTravelUtils
{
	// 모든 원격 talker 등록 해제. bStopLocalVoice=true면 로컬 캡처도 중단.
	void ShutdownVoiceForTravel(UWorld* World, bool bStopLocalVoice);

	// 씬 전환 후 원격 talker 재등록. 자신의 PS는 제외.
	void RestoreRemoteTalkers(UWorld* World, const APlayerController* LocalController);

	// Transient 패키지 소유 VoipListenerSynthComponent 강제 해제.
	void UnregisterTransientVoipComps();
}
