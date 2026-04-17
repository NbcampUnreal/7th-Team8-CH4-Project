// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HeistUIInputModePolicy.generated.h"

UENUM(BlueprintType)
enum class EHeistUIInputMode : uint8
{
	Lobby,
	Briefing,
	Match
};
