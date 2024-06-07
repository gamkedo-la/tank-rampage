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

	TRCORE_API TOptional<FGroundData> GetGroundData(const AActor& Actor, const TOptional<FVector>& PositionOverride = {});

	TRCORE_API void ResetActorToGround(const FGroundData& GroundData, AActor& Actor, float AdditionalZOffset = 0.0f);
}

namespace TR::CollisionChannel
{
	inline constexpr ECollisionChannel GroundObjectType = ECollisionChannel::ECC_GameTraceChannel1;
	inline constexpr ECollisionChannel ExplosionDamageTraceType = ECollisionChannel::ECC_GameTraceChannel2;
	inline constexpr ECollisionChannel MissileHomingTargetTraceType = ECollisionChannel::ECC_GameTraceChannel3;
	inline constexpr ECollisionChannel ProjectileObjectType = ECollisionChannel::ECC_GameTraceChannel4;
}

namespace TR::CollisionProfile
{
	inline constexpr const TCHAR* Projectile = TEXT("Projectile");
	inline constexpr const TCHAR* Tank = TEXT("Tank");
	inline constexpr const TCHAR* NoCollision = TEXT("NoCollision");
	inline constexpr const TCHAR* StaticOverlapOnlyTank = TEXT("OverlapTankStatic");
	inline constexpr const TCHAR* DynamicOverlapOnlyTank = TEXT("OverlapTankDynamic");
}
