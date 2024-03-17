// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/CollisionUtils.h"

#include "Utils/ObjectUtils.h"

#include "TRCoreLogging.h"
#include "Logging/LoggingUtils.h"

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
