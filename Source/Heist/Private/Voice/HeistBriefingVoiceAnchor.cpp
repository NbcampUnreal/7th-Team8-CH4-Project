
#include "Voice/HeistBriefingVoiceAnchor.h"

AHeistBriefingVoiceAnchor::AHeistBriefingVoiceAnchor()
{
	VoiceAttachRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VoiceAttachRoot"));
	RootComponent = VoiceAttachRoot;
}
