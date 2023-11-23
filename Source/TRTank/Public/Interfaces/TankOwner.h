// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TankOwner.generated.h"

class ABaseTankPawn;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTankOwner : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TRTANK_API ITankOwner
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual ABaseTankPawn* GetControlledTank() const = 0;
};
