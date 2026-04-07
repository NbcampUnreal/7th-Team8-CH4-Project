#include "DebugWidgetBase.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"

FString UDebugWidgetBase::GetNetModeString() const
{
	ENetMode NetMode = GetWorld()->GetNetMode();
	switch (NetMode)
	{
		case NM_Standalone: return TEXT("Standalone");
		case NM_DedicatedServer: return TEXT("Dedicated Server");
		case NM_ListenServer: return TEXT("Listen Server");
		case NM_Client: return TEXT("Client");
		default: return TEXT("Unknown");
	}
}

FString UDebugWidgetBase::GetLocalRoleString() const
{
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn) return TEXT("No Pawn");

	return RoleToString(OwningPawn->GetLocalRole());
}

float UDebugWidgetBase::GetCurrentPing() const
{
	APlayerController* PC = GetOwningPlayer();
	if (PC && PC->PlayerState)
	{
		return PC->PlayerState->GetPingInMilliseconds();
	}
	return 0.0f;
}

float UDebugWidgetBase::GetCurrentFPS() const
{
	return DisplayFPS;
}

FString UDebugWidgetBase::RoleToString(ENetRole Role) const
{
	switch (Role)
	{
		case ROLE_Authority: return TEXT("Authority (Server)");
		case ROLE_AutonomousProxy: return TEXT("Autonomous Proxy (Owner)");
		case ROLE_SimulatedProxy: return TEXT("Simulated Proxy (Remote)");
		case ROLE_None: return TEXT("None");
		default: return TEXT("Unknown");
	}
}

void UDebugWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	TimerCounter += InDeltaTime; 
	FrameCounter++; 
	if (TimerCounter >= 0.5f)
	{ 
		DisplayFPS = (float)FrameCounter / TimerCounter;

		TimerCounter = 0.0f;
		FrameCounter = 0;
	}
}
