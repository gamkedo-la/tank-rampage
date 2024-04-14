// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TankEventsSubsystem.generated.h"

class ABaseTankPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTankDestroyed, ABaseTankPawn*, DestroyedTank, AController*,  DestroyedBy, AActor*, DestroyedWith);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemySpawned, APawn*, Enemy);

/**
 * 
 */
UCLASS()
class TRTANK_API UTankEventsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnTankDestroyed OnTankDestroyed;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnEnemySpawned OnEnemySpawned;
};
