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

/*
* Gets available player unlocks based on the next level and also applies them to the player's inventory.
* At least one new unlock will be returned in the available options as long as there is at least one new one at the given level and player can upgrade to it.
* A player can be given an unlock if it is a level 1 item and player does not yet have that item in their inventory, or the player has the item and the next level of the 
* upgrade is the one being considered.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ULevelUnlocksComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULevelUnlocksComponent();

	std::optional<FLevelUnlocksContext> GetNextLevelUnlockOptions(APawn* Pawn, int32 NextLevel) const;

	UFUNCTION(BlueprintCallable)
	void ApplyLevelUnlock(APawn* Pawn, const FLevelUnlock& Unlock) const;

	void SetLevelUnlocks(const TArray<FLevelUnlocksConfig>& InLevelUnlocks);

protected:
	virtual void BeginPlay() override;

private:

	void GiveLocalPlayerFirstWeapon() const;
	void DetermineAvailableOptionCounts(int32 NextLevel, int32& NumCurrent, int32& NumAvailableOptions) const;
	TArray<FLevelUnlock> GetAvailableUnlocks(const int32 NextLevel, const UItemInventory* ItemInventory, int32& NumCurrent, const int32 NumAvailableOptions) const;

	std::optional<FLevelUnlocksContext> GetFirstLevelUnlockOptions() const;
	FLevelUnlocksContext GetLevelUnlocksContext(int32 NextLevel, const TArray<FLevelUnlock>& TotalOptions, int32 NumAvailableOptions) const;

	void PopulateViableUnlockOptions(const TArray<UItem*>& CurrentItems, const TArray<FLevelUnlock>& TotalOptions, TArray<FLevelUnlock>& OutOptions) const;
	UItemInventory* GetItemInventory(APawn* Pawn) const;

private:

	UPROPERTY(Transient)
	TArray<FLevelUnlocksConfig> LevelUnlocks;

	mutable std::default_random_engine Rng;
};

#pragma region Inline Definitions

inline void ULevelUnlocksComponent::SetLevelUnlocks(const TArray<FLevelUnlocksConfig>& InLevelUnlocks)
{
	this->LevelUnlocks = InLevelUnlocks;
}

#pragma endregion Inline Definitions
