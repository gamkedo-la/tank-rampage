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

private:
	UPROPERTY(Category = "Effects | Hit", EditDefaultsOnly)
	float ShockwaveDuration{ 5.0f };
};
