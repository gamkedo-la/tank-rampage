// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankTrackComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"
#include "VisualLogger/VisualLogger.h"

void UTankTrackComponent::SetThrottle(float InThrottle)
{
	const auto Throttle = FMath::Clamp(InThrottle, -1.0f, 1.0f);

	auto ForceApplied = GetForwardVector() * Throttle * TrackMaxDrivingForce;
	const auto& ForceLocation = GetComponentLocation();

	auto RootComponent = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());

	if (!RootComponent)
	{
		UE_LOG(LogTRTank, Error, TEXT("%s-%s: Owner root component %s is not a primitive component - unable to move"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(GetOwner()->GetRootComponent()));
		return;
	}

	RootComponent->AddForceAtLocation(ForceApplied, ForceLocation);
}
