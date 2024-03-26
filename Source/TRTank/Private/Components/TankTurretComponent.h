// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"

#include "Containers/TimedCircularBuffer.h"

#include "TankTurretComponent.generated.h"

/**
 * Rotates the aim to yaw the barrel toward aiming location.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), HideCategories = ("Collision"))
class UTankTurretComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	UTankTurretComponent();

	/*
	* Rotate the barrel at given relative speed.
	*
	* @param RelativeSpeed -1 is max downward movement and 1 is max upward movement
	*/
	bool Rotate(double RelativeSpeed);

protected:
	virtual void InitializeComponent() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = Setup)
	float MaxDegreesPerSecond{ 45.0f };

	UPROPERTY(EditDefaultsOnly, Category = Oscillations)
	bool bEnableOscillationDetection {};

	UPROPERTY(EditDefaultsOnly, Category = Oscillations, meta = (ClampMin = "1"))
	int32 NumSamples{ 60 };

	UPROPERTY(EditDefaultsOnly, Category = Oscillations, meta = (ClampMin = "0"))
	float OscillationThresholdDegrees { 2.0f };

	using BufferType = TR::TTimedCircularBuffer<double>;
	TUniquePtr<BufferType> OscillationsBuffer{};
};
