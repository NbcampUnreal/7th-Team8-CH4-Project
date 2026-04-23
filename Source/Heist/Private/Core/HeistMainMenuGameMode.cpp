#include "Core/HeistMainMenuGameMode.h"

#include "Systems/Audio/HeistAudioSubsystem.h"

#include "Engine/World.h"

void AHeistMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UHeistAudioSubsystem* AudioSubsystem = World->GetSubsystem<UHeistAudioSubsystem>())
		{
			AudioSubsystem->TransitionToBGM(EHeistSoundType::BGM_MainMenu);
		}
	}
}
