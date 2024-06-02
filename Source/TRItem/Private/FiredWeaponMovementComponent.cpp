// Fill out your copyright notice in the Description page of Project Settings.


#include "FiredWeaponMovementComponent.h"

#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FiredWeaponMovementComponent)

void UFiredWeaponMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	// If we hit the actor's owner (i.e. the tank that fired the projectile), then check to make sure that we are allowed to do self-collision
	if (auto HitActor = Hit.GetActor(); HitActor && GetOwner() && GetOwner()->GetOwner() == HitActor && !bCanDamageOwner)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRItem, Log, TEXT("%s-%s: Skipping self collision on %s"), *GetOwner()->GetName(), *GetName(), *HitActor->GetName());
		return;
	}

	Super::HandleImpact(Hit, TimeSlice, MoveDelta);
}
