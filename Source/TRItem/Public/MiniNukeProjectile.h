// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "MiniNukeProjectile.generated.h"

/**
 * 
 */
UCLASS()
class TRITEM_API AMiniNukeProjectile : public AProjectile
{
	GENERATED_BODY()
	
protected:
	virtual void ApplyPostProcessEffects() override;

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
};
