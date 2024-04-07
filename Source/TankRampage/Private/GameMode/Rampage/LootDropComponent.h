// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "LootDropComponent.generated.h"

class ABasePickup;
class ABaseTankPawn;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ULootDropComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULootDropComponent();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnTankDestroyed(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith);

	UFUNCTION()
	void OnXPLevelUp(int32 NewLevel);

	void SpawnLoot(const AController* Owner, const FVector& BaseSpawnLocation) const;

	const ABasePickup* SpawnLoot(const AController* Owner, const FVector& SpawnLocation, UClass* PickupClass) const;
	FVector GetSpawnLocation(const FVector& BaseLocation) const;

	bool ShouldSpawnLootClass(const UClass* PickupClass) const;


private:
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TArray<TSubclassOf<ABasePickup>> LootClasses{};

	// TODO: Switch to curve table with possibility of guaranteed drops per level
	UPROPERTY(EditDefaultsOnly)
	float DropProbability{ 0.05f };

	UPROPERTY(EditDefaultsOnly)
	float SpawnRadius{ 300.0f };

	int32 CurrentLevel{};
	int32 EnemiesDestroyedThisLevel{};
};
