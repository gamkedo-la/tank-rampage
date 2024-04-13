// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


namespace TR::CollisionUtils
{
	TRCORE_API FBox GetAABB(const AActor& Actor);

	TRCORE_API FBox GetAABB(const USceneComponent& Component);
}

namespace TR::CollisionChannel
{
	inline constexpr ECollisionChannel GroundObjectType = ECollisionChannel::ECC_GameTraceChannel1;
}
