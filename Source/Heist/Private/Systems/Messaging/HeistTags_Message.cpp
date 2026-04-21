#include "Systems/Messaging/HeistTags_Message.h"

namespace HeistMessageTags
{
	UE_DEFINE_GAMEPLAY_TAG(Message_SystemMenu_Toggle, "Message.SystemMenu.Toggle");

	UE_DEFINE_GAMEPLAY_TAG(Message_Lobby_PlayersChanged, "Message.Lobby.PlayersChanged");
	UE_DEFINE_GAMEPLAY_TAG(Message_Lobby_ReadyStateChanged, "Message.Lobby.ReadyStateChanged");
	UE_DEFINE_GAMEPLAY_TAG(Message_Lobby_InviteCodeChanged, "Message.Lobby.InviteCodeChanged");
	UE_DEFINE_GAMEPLAY_TAG(Message_Phase_Changed, "Message.Phase.Changed");
	UE_DEFINE_GAMEPLAY_TAG(Message_Phase_TimeUpdated, "Message.Phase.TimeUpdated");

	UE_DEFINE_GAMEPLAY_TAG(Message_Ping, "Message.Ping");
	UE_DEFINE_GAMEPLAY_TAG(Message_Ping_Danger, "Message.Ping.Danger");
	UE_DEFINE_GAMEPLAY_TAG(Message_Ping_Move, "Message.Ping.Move");
	UE_DEFINE_GAMEPLAY_TAG(Message_Ping_Item, "Message.Ping.Item");
	UE_DEFINE_GAMEPLAY_TAG(Message_Ping_Rescue, "Message.Ping.Rescue");

	UE_DEFINE_GAMEPLAY_TAG(Message_Alarm, "Message.Alarm");
	UE_DEFINE_GAMEPLAY_TAG(Message_Alarm_Theft, "Message.Alarm.Theft");
	UE_DEFINE_GAMEPLAY_TAG(Message_Alarm_GPS, "Message.Alarm.GPS");
	UE_DEFINE_GAMEPLAY_TAG(Message_Alarm_Global, "Message.Alarm.Global");
	
	UE_DEFINE_GAMEPLAY_TAG(Message_Briefing_ContextReady, "Message.Briefing.ContextReady");
	UE_DEFINE_GAMEPLAY_TAG(Message_Briefing_End, "Message.Briefing.End");

	UE_DEFINE_GAMEPLAY_TAG(Message_PlayHUD_Ready, "Message.PlayHUD.Ready");
	
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_SoundDetected, "Message.UI.SoundDetected");
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_FlashlightAlert, "Message.UI.FlashlightAlert");
	UE_DEFINE_GAMEPLAY_TAG(Message_Voice_TalkingStateChanged, "Message.Voice.TalkingStateChanged");

	UE_DEFINE_GAMEPLAY_TAG(Message_Travel_SeamlessStart, "Message.Travel.SeamlessStart");
	UE_DEFINE_GAMEPLAY_TAG(Message_Travel_SeamlessEnd, "Message.Travel.SeamlessEnd");
}
