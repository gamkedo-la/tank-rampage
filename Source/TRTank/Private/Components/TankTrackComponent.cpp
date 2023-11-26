// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankTrackComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"
#include "VisualLogger/VisualLogger.h"

void UTankTrackComponent::SetThrottle(float InThrottle)
{
	// TODO: Clamp actual throttle
	UE_LOG(LogTemp, Warning, TEXT("%s-%s: SetThrottle: %f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), InThrottle);

	auto ForceApplied = GetForwardVector() * InThrottle * TrackMaxDrivingForce;
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
