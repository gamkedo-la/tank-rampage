// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameStatisticsSubsystem.generated.h"

class UItem;
struct FGameplayTagContainer;
class UItemInventory;
struct FItemConfigData;

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

	UFUNCTION(BlueprintPure)
	void GetItemMostKillsAttributedTo(UItem*& Item, int32& Count) const;

protected:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:

	UFUNCTION()
	void OnTankDestroyed(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith);

	void OnItemGameplayTagsChanged(UItem* Item, const TArray<APawn*>& AffectedPawns, const FGameplayTagContainer& Tags, bool bAdded);

	UFUNCTION()
	void OnInventoryItemAdded(const UItemInventory* Inventory, const FName& Name, int32 Index, const FItemConfigData& ItemConfigData);

	bool ShouldRecordUpdate() const;

	void AttributeEnemyKillItem(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith);

private:
	int32 EnemiesKilled{};

	UPROPERTY(Transient)
	TMap<UItem*, int32> ItemKills{};

	UPROPERTY(Transient)
	TSet<APawn*> DisabledEnemies{};

	UPROPERTY(Transient)
	TObjectPtr<UItem> DisabledItemWeaponCredit{};
};
