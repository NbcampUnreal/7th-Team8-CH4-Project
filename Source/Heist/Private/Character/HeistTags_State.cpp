#include "Character/HeistTags_State.h"

namespace HeistStateTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_Sneaking, "State.Sneaking");
	UE_DEFINE_GAMEPLAY_TAG(State_Stunned, "State.Stunned");
	UE_DEFINE_GAMEPLAY_TAG(State_Knockback, "State.Knockback");
	UE_DEFINE_GAMEPLAY_TAG(State_ActionDisabled, "State.ActionDisabled");
	UE_DEFINE_GAMEPLAY_TAG(State_MoveDisabled, "State.MoveDisabled");
	UE_DEFINE_GAMEPLAY_TAG(State_RotationDisabled, "State.RotationDisabled");
	UE_DEFINE_GAMEPLAY_TAG(State_Channeling, "State.Channeling");

	UE_DEFINE_GAMEPLAY_TAG(State_Thief_Normal, "State.Thief.Normal");
	UE_DEFINE_GAMEPLAY_TAG(State_Thief_Injured, "State.Thief.Injured");
	UE_DEFINE_GAMEPLAY_TAG(State_Thief_Cuffed, "State.Thief.Cuffed");
	UE_DEFINE_GAMEPLAY_TAG(State_Thief_Escorted, "State.Thief.Escorted");
	UE_DEFINE_GAMEPLAY_TAG(State_Thief_Out, "State.Thief.Out");

	UE_DEFINE_GAMEPLAY_TAG(State_Police_Normal, "State.Police.Normal");
	UE_DEFINE_GAMEPLAY_TAG(State_Police_Escorting, "State.Police.Escorting");

	UE_DEFINE_GAMEPLAY_TAG(Zone_Indoor, "Zone.Indoor");
	UE_DEFINE_GAMEPLAY_TAG(Zone_Outdoor, "Zone.Outdoor");
	UE_DEFINE_GAMEPLAY_TAG(State_Thief_InFlashlight, "State.Thief.InFlashlight");
}
