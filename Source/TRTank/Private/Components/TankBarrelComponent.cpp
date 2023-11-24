// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankBarrelComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"
#include "VisualLogger/VisualLogger.h"

void UTankBarrelComponent::Elevate(float DegreesPerSecond)
{
	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: Elevate - DegreesPerSecond=%f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), DegreesPerSecond);
}
