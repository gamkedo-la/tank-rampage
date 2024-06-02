// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankCrashComponent.h"

#include "Components/TankTurretComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"

#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TankCrashComponent)

void UTankCrashComponent::RegisterCollisions()
{
	if (!ensureMsgf(GetHitSfx(), TEXT("%s: Has no configured TankCrashSfx"), *GetName()))
	{
		return;
	}

	auto Owner = GetOwner();
	check(Owner);

	// Register for the body and turret
	if (auto TankBody = Cast<UPrimitiveComponent>(Owner->GetRootComponent()); 
		ensureMsgf(TankBody, TEXT("%s: Could not find tank body component (root primitive component")))
	{
		RegisterComponent(TankBody);
	}

	if (auto Turret = Owner->FindComponentByClass<UTankTurretComponent>();
		ensureMsgf(Turret, TEXT("%s: Could not find tank turret component")))
	{
		RegisterComponent(Turret);
	}
}
