// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Controllers/BaseAIController.h"
#include "Interfaces/TankOwner.h"

#include "TankAIController.generated.h"

/**
 * 
 */
UCLASS()
class TRAI_API ATankAIController : public ABaseAIController, public ITankOwner
{
	GENERATED_BODY()
	
public:
	// Inherited via ITankOwner
	ABaseTankPawn* GetControlledTank() const override;

private:
	ABaseTankPawn* GetPlayerTank() const;

};
