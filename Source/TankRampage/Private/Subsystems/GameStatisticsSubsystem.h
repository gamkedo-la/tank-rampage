// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameStatisticsSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class UGameStatisticsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure)
	int32 GetEnemiesKilled() const { return EnemiesKilled; }

protected:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:

	UFUNCTION()
	void OnTankDestroyed(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith);

	bool ShouldRecordUpdate() const;


private:
	int32 EnemiesKilled{};
	
};
