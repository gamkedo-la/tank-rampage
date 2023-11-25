// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "TankTurretComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), HideCategories = ("Collision"))
class UTankTurretComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	/*
	* Rotate the barrel at given relative speed.
	*
	* @param RelativeSpeed -1 is max downward movement and 1 is max upward movement
	*/
	void Rotate(float RelativeSpeed);

private:
	UPROPERTY(EditAnywhere, Category = Setup)
	float MaxDegreesPerSecond{ 45.0f };
};
