// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


namespace TR::CollisionUtils
{
	TRCORE_API FBox GetAABB(const AActor& Actor);

	TRCORE_API FBox GetAABB(const USceneComponent& Component);

	TRCORE_API struct FGroundData
	{
		FVector Location;
		FVector Normal;
	};

	TRCORE_API float GetActorHalfHeight(const AActor& Actor);

	TRCORE_API TOptional<FGroundData> GetGroundData(const AActor& Actor);

	TRCORE_API void ResetActorToGround(const FGroundData& GroundData, AActor& Actor);
}

namespace TR::CollisionChannel
{
	inline constexpr ECollisionChannel GroundObjectType = ECollisionChannel::ECC_GameTraceChannel1;
	inline constexpr ECollisionChannel ExplosionDamageTraceType = ECollisionChannel::ECC_GameTraceChannel2;
}
