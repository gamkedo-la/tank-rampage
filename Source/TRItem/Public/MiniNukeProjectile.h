// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "MiniNukeProjectile.generated.h"

class UCameraShakeBase;

/**
 * 
 */
UCLASS()
class TRITEM_API AMiniNukeProjectile : public AProjectile
{
	GENERATED_BODY()
	
protected:
	virtual void ApplyPostHitEffects(const FHitResult& HitInfo, const FProjectileDamageParams& DamageParams) override;

private:
	void ApplyPostProcessEffects();
	void ApplyCameraShake(const FHitResult& HitInfo, const FProjectileDamageParams& DamageParams) const;

protected:
	UPROPERTY(Category = "Effects | Hit", EditDefaultsOnly, BlueprintReadWrite, meta=(ClampMin="0.1"))
	float PostProcessMinDuration{ 5.0f };

	UPROPERTY(Category = "Effects | Hit", EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0.1"))
	float PostProcessMaxDuration{ 10.0f };

	UPROPERTY(Category = "Effects | Hit", EditDefaultsOnly, BlueprintReadWrite)
	float PostProcessInterpolateInterval{ 0.1f };

private:
	UPROPERTY(Category = "Effects | Hit", EditDefaultsOnly)
	bool bEnablePostProcess{};

	UPROPERTY(Category = "Effects | Hit", EditDefaultsOnly)
	FPostProcessSettings PostProcessSettings{};

	UPROPERTY(Category = "Effects | Hit | Shake", EditDefaultsOnly)
	TSubclassOf<UCameraShakeBase> ExplosionCameraShake{};

	UPROPERTY(Category = "Effects | Hit | Shake", EditDefaultsOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CameraShakeFalloff{ 1.0f };

	/**
	* Multiplier on damage inner radius for full effect of the camera shake
	*/
	UPROPERTY(Category = "Effects | Hit | Shake", EditDefaultsOnly, meta = (ClampMin = "1.0"))
	float CameraShakeInnerRadiusMultiplier{ 1.0f };

	/**
	* Multiplier on damage outer radius for full effect of the camera shake
	*/
	UPROPERTY(Category = "Effects | Hit | Shake", EditDefaultsOnly, meta = (ClampMin = "1.0"))
	float CameraShakeOuterRadiusMultiplier{ 1.0f };
};
