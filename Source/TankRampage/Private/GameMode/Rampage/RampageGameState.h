// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
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
	float GetLevelPercent() const { return static_cast<float>(CurrentXP) / LevelUpXP; };

	UFUNCTION(BlueprintPure)
	int32 GetTotalXP() const { return TotalXP; }

private:

	friend class ARampageGameMode;

	int32 Level{};
	int32 CurrentXP{};
	int32 LevelUpXP{};
	int32 TotalXP{};
};
