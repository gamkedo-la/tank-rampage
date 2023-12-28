// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelUnlocksContext.h"

#include <optional>
#include <random>

#include "LevelUnlocksComponent.generated.h"

class UItem;
class UItemInventory;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ULevelUnlocksComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULevelUnlocksComponent();

	std::optional<FLevelUnlocksContext> GetNextLevelUnlockOptions(APawn* Pawn, int32 NextLevel) const;

	UFUNCTION(BlueprintCallable)
	void ApplyLevelUnlock(APawn* Pawn, const FLevelUnlock& Unlock) const;

protected:
	virtual void BeginPlay() override;

private:
	std::optional<FLevelUnlocksContext> GetFirstLevelUnlockOptions() const;
	FLevelUnlocksContext GetLevelUnlocksContext(int32 NextLevel, const TArray<FLevelUnlock>& TotalOptions, int32 NumAvailableOptions) const;

	void PopulateViableUnlockOptions(const TArray<UItem*>& CurrentItems, const TArray<FLevelUnlock>& TotalOptions, TArray<FLevelUnlock>& OutOptions) const;
	UItemInventory* GetItemInventory(APawn* Pawn) const;

private:

	// TODO: A DataTable is a better option for maintaining this
	// Then in begin play could convert it to this TArray<FLevelUnlocksConfig> structure
	UPROPERTY(EditDefaultsOnly)
	TArray<FLevelUnlocksConfig> LevelUnlocks;

	mutable std::default_random_engine Rng;
};

