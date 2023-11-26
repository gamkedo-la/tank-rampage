// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "TankBarrelComponent.generated.h"

/**
 * Pitches up and down for aiming the main gun.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), HideCategories = ("Collision"))
class UTankBarrelComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	/*
	* Elevate the barrel at given relative speed.
	* 
	* @param RelativeSpeed -1 is max downward movement and 1 is max upward movement
	*/
	void Elevate(float RelativeSpeed);

private:
	UPROPERTY(EditDefaultsOnly, Category = Setup)
	float MaxDegreesPerSecond{ 20.0f };
	
	UPROPERTY(EditDefaultsOnly, Category = Setup)
	float MinElevationDegrees{ -10.0f };

	UPROPERTY(EditDefaultsOnly, Category = Setup)
	float MaxElevationDegrees{ 30.0f };
};
