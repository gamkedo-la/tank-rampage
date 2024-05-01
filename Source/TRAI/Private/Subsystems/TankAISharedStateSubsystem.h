// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TankAISharedStateSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class UTankAISharedStateSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	FVector LastPlayerSeenLocation{ EForceInit::ForceInitToZero };
	float LastPlayerSeenTime{ -1.0f };
};
