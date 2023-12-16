// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "FiredWeaponMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class TRITEM_API UFiredWeaponMovementComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()
	
public:
	void SetCanDamageOwner(bool bInCanDamageOwner);

protected:
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.f, const FVector& MoveDelta = FVector::ZeroVector) override;

private:

	bool bCanDamageOwner{};
};

#pragma region Inline Definitions

inline void UFiredWeaponMovementComponent::SetCanDamageOwner(bool bInCanDamageOwner)
{
	bCanDamageOwner = bInCanDamageOwner;
}

#pragma endregion Inline Definitions
