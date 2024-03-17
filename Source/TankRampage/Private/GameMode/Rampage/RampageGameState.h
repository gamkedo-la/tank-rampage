// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "LevelUnlocksContext.h"

#include "RampageGameState.generated.h"

/**
 * 
 */
UCLASS()
class ARampageGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	int32 GetCurrentLevel() const { return Level; }

	UFUNCTION(BlueprintPure)
	float GetLevelPercent() const { return static_cast<float>(TotalXP - PreviousLevelXP) / (LevelUpXP - PreviousLevelXP); };

	UFUNCTION(BlueprintPure)
	int32 GetTotalXP() const { return TotalXP; }

	UFUNCTION(BlueprintPure)
	const FLevelUnlocksContext& GetNextLevelUnlocks() const { return NextLevelUnlocks; }

	UFUNCTION(BlueprintCallable)
	void ApplyLevelUnlock(APawn* Pawn, const FLevelUnlock& Unlock);

	UFUNCTION(BlueprintPure)
	float GetFirstEnemySpawnTime() const { return FirstEnemySpawnTime; }

	UFUNCTION(BlueprintPure)
	bool SpawnersAreValid() const { return bSpawnersAreValid; }

private:

	friend class ARampageGameMode;

	FLevelUnlocksContext NextLevelUnlocks{};

	int32 Level{};
	int32 PreviousLevelXP{};
	int32 LevelUpXP{};
	int32 TotalXP{};
	float FirstEnemySpawnTime{};
	bool bSpawnersAreValid{};
};
