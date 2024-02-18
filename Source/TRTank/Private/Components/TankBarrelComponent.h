// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"

#include "Containers/TimedCircularBuffer.h"

#include "TankBarrelComponent.generated.h"

/**
 * Pitches up and down for aiming the main gun.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), HideCategories = ("Collision"))
class UTankBarrelComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	UTankBarrelComponent();

	/*
	* Elevate the barrel at given relative speed.
	* 
	* @param RelativeSpeed -1 is max downward movement and 1 is max upward movement
	*/
	bool Elevate(float RelativeSpeed);

protected:
	virtual void InitializeComponent() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = Setup)
	float MaxDegreesPerSecond{ 20.0f };
	
	UPROPERTY(EditDefaultsOnly, Category = Setup)
	float MinElevationDegrees{ -10.0f };

	UPROPERTY(EditDefaultsOnly, Category = Setup)
	float MaxElevationDegrees{ 30.0f };

	UPROPERTY(EditDefaultsOnly, Category = Oscillations)
	bool bEnableOscillationDetection {};

	UPROPERTY(EditDefaultsOnly, Category = Oscillations, meta = (ClampMin = "1"))
	int32 NumSamples{ 60 };

	UPROPERTY(EditDefaultsOnly, Category = Oscillations, meta = (ClampMin = "0"))
	float OscillationThresholdDegrees{ 0.5f };

	using BufferType = TR::TTimedCircularBuffer<float>;
	TUniquePtr<BufferType> OscillationsBuffer{};
};
