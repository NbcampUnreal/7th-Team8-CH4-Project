#pragma once

#include "CoreMinimal.h"
#include "Core/HeistMatchTypes.h"
#include "UObject/Interface.h"
#include "HeistBriefingWidgetInterface.generated.h"

class UHeistBriefingPlayerComponent;
class UHeistBriefingDrawingSyncComponent;

/** UObject 래퍼. 인터페이스 등록용이며 직접 사용하지 않는다. */
UINTERFACE(MinimalAPI)
class UHeistBriefingWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 브리핑 위젯 초기화 인터페이스.
 *
 * PlayerController(Heist 모듈)가 HeistUI 모듈의 구체 위젯 타입을 직접 참조하지 않고
 * 초기화를 위임하기 위해 정의한다. Heist <-> HeistUI 순환 의존을 방지한다.
 *
 * 이 인터페이스를 구현하는 위젯은 InitializeForBriefing()에서
 * BriefingPlayerComponent / DrawingSyncComponent / ViewMode를 수신해
 * 자체 상태를 초기화해야 한다.
 */
class HEIST_API IHeistBriefingWidgetInterface
{
	GENERATED_BODY()

public:
	/**
	 * 브리핑 컨텍스트가 준비됐을 때 PlayerController가 호출한다.
	 * DrawingBoard 복제 완료(클라이언트) 또는 서버 세팅 완료(리슨 호스트) 직후 발동된다.
	 *
	 * @param InBriefingPlayerComponent  플레이어별 브리핑 입력 진입점 (서버 RPC 전송용)
	 * @param InDrawingSyncComponent     스트로크 이력 저장소 (실시간 렌더 구독용)
	 * @param InViewMode                 이 플레이어의 팀 뷰 모드 (Thief / Police)
	 */
	virtual void InitializeForBriefing(
		UHeistBriefingPlayerComponent* InBriefingPlayerComponent,
		UHeistBriefingDrawingSyncComponent* InDrawingSyncComponent,
		EHeistBriefingViewMode InViewMode) = 0;
};
