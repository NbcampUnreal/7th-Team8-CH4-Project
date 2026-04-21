#include "AbilitySystem/HeistTags_Event.h"

namespace HeistEventTags
{
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit, "Event.Hit");
	UE_DEFINE_GAMEPLAY_TAG(Event_Healed, "Event.Healed");
	UE_DEFINE_GAMEPLAY_TAG(Event_CuffingComplete, "Event.CuffingComplete");
	UE_DEFINE_GAMEPLAY_TAG(Event_EscortStarted, "Event.EscortStarted");
	UE_DEFINE_GAMEPLAY_TAG(Event_EscortInterrupted, "Event.EscortInterrupted");
	UE_DEFINE_GAMEPLAY_TAG(Event_ArrivedAtCar, "Event.ArrivedAtCar");
	UE_DEFINE_GAMEPLAY_TAG(Event_Arrested, "Event.Arrested");	// 경찰차 도착 시 서버 authoritative 체포 확정과 병행해 발송하는 후속 반응용 이벤트
	UE_DEFINE_GAMEPLAY_TAG(Event_KickHit, "Event.KickHit");
	UE_DEFINE_GAMEPLAY_TAG(Event_StunExpired, "Event.StunExpired");
	UE_DEFINE_GAMEPLAY_TAG(Event_Input_Move, "Event.Input.Move");
	UE_DEFINE_GAMEPLAY_TAG(Event_SoundDetected, "Event.SoundDetected");
	UE_DEFINE_GAMEPLAY_TAG(Event_CarryStarted, "Event.CarryStarted");
	UE_DEFINE_GAMEPLAY_TAG(Event_CarryUpdate, "Event.CarryUpdate");
	UE_DEFINE_GAMEPLAY_TAG(Event_CarryDrop, "Event.CarryDrop");
	UE_DEFINE_GAMEPLAY_TAG(Event_EngineStarted, "Event.EngineStarted");
}
