// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/CollisionUtils.h"

#include "Utils/ObjectUtils.h"

#include "TRCoreLogging.h"
#include "Logging/LoggingUtils.h"

#include "VisualLogger/VisualLogger.h"

using namespace TR;

namespace
{
	FBox DefaultGetAABB(const AActor& Actor);
}

FBox TR::CollisionUtils::GetAABB(const AActor& Actor)
{
	if (!ObjectUtils::IsClassDefaultObject(&Actor))
	{
		return DefaultGetAABB(Actor);
	}

	// Cannot use GetActorBounds on a CDO as it only calculates the bounds of registered components and this only happens in InitializeComponent
	// Need to use USceneComponent::CalcBounds

	const USceneComponent* RootSceneComponent = Actor.FindComponentByClass<const USceneComponent>();
	if (!RootSceneComponent)
	{
		UE_LOG(LogTRCore, Warning, TEXT("%s: Could not find a USceneComponent to calculate bounds on - returning zero bounds"), *Actor.GetName());
		return FBox{ EForceInit::ForceInitToZero };
	}

	const auto& Bounds = RootSceneComponent->CalcLocalBounds().GetBox();

	UE_LOG(LogTRCore, VeryVerbose, TEXT("%s: GetAABB()=%s"), *Actor.GetName(), *Bounds.ToString());

	return Bounds;
}

FBox TR::CollisionUtils::GetAABB(const USceneComponent& Component)
{
	if (!ObjectUtils::IsClassDefaultObject(&Component))
	{
		// See UKismetSystemLibrary::GetComponentBounds
		const auto& Bounds = Component.Bounds;
		return Bounds.GetBox();
	}

	return Component.CalcLocalBounds().GetBox();
}

float TR::CollisionUtils::GetActorHalfHeight(const AActor& Actor)
{
	FVector ActorOrigin, BoxExtent;

	Actor.GetActorBounds(true, ActorOrigin, BoxExtent, false);

	return BoxExtent.Z;
}

TOptional<CollisionUtils::FGroundData> TR::CollisionUtils::GetGroundData(const AActor& Actor, const TOptional<FVector>& PositionOverride)
{
	auto World = Actor.GetWorld();
	if (!World)
	{
		return {};
	}

	// Find elevation at TargetLocation
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(&Actor);

	const auto& TargetLocation = PositionOverride ? *PositionOverride : Actor.GetActorLocation();

	const auto TraceStart = TargetLocation + FVector(0, 0, GetActorHalfHeight(Actor));
	const auto TraceEnd = TargetLocation - FVector(0, 0, 1000);

	FHitResult HitResult;

	if (!World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECollisionChannel::ECC_Visibility,
		CollisionQueryParams))
	{
		return {};
	}

	return FGroundData
	{
		.Location = HitResult.Location,
		.Normal = HitResult.Normal
	};
}

void TR::CollisionUtils::ResetActorToGround(const FGroundData& GroundData, AActor& Actor, float AdditionalZOffset)
{
	const FRotator GroundRotation = GroundData.Normal.ToOrientationRotator();

	const FRotator ResetRotation(90 - GroundRotation.Pitch, Actor.GetActorRotation().Yaw, 0);

	const auto OffsetHeight = GetActorHalfHeight(Actor) + AdditionalZOffset;
	const FVector ResetLocation = GroundData.Location + GroundData.Normal * OffsetHeight;

	Actor.SetActorTransform(FTransform(ResetRotation, ResetLocation), false, nullptr, ETeleportType::ResetPhysics);
}

namespace
{
	FBox DefaultGetAABB(const AActor& Actor)
	{
		FVector ActorOrigin;
		FVector BoxExtent;

		Actor.GetActorBounds(true, ActorOrigin, BoxExtent, false);

		// ActorOrigin aligns with the AABB Origin correctly
		return FBox::BuildAABB(ActorOrigin, BoxExtent);
	}
}
