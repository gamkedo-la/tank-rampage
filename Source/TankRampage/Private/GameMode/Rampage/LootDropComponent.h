// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "LootDropComponent.generated.h"

class ABasePickup;
class ABaseTankPawn;
class UCurveTable;
struct FRealCurve;

USTRUCT()
struct FLootConfig
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ABasePickup> Class{};

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCurveTable> DropCurveTable{};
};

USTRUCT()
struct FLootLevelData
{
	GENERATED_USTRUCT_BODY()

	FRealCurve* Curve{};

	int32 Awarded{};
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ULootDropComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULootDropComponent();

protected:
	virtual void BeginPlay() override;

private:
	struct FSpawnContext;

	UFUNCTION()
	void OnTankDestroyed(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith);

	UFUNCTION()
	void OnXPLevelUp(int32 NewLevel);

	void SpawnLoot(const AController* Owner, const FVector& BaseSpawnLocation);

	const ABasePickup* SpawnLoot(const AController* Owner, const FVector& SpawnLocation, UClass* PickupClass) const;
	FVector GetSpawnLocation(FSpawnContext& SpawnContext, const FVector& BaseLocation, FVector& InitialSpawnLocation) const;

	FVector GetInitialSpawnLocation(FSpawnContext& SpawnContext, const FVector& BaseLocation) const;
	bool GetCollisionFreeSpawnLocation(FSpawnContext& SpawnContext, FVector& SpawnLocation) const;
	TOptional<FBox> GetPickupBounds(const TSubclassOf<ABasePickup>& PickupClass) const;
	TOptional<FBox> GetPickupBounds(const ABasePickup& Pickup) const;
	bool IsOverlappingExistingSpawns(const FSpawnContext& SpawnContext, const FVector& InitialSpawnLocation) const;

	FVector GroundSpawnLocation(const TSubclassOf<ABasePickup>& PickupClass, const FVector& Location) const;

	bool ShouldSpawnLootClass(const FLootConfig& LootConfig) const;

	bool ValidateConfig();
	bool ValidateCurveTable(UCurveTable* CurveTable) const;

	void InitializeLevelData(int32 Level);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TArray<FLootConfig> LootConfigs{};

	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	float SpawnRadius{ 300.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	int32 CollisionTestMaxIterations{ 5 };

	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	int32 MultipleLootOverlapCheckMaxIterations{ 5 };

	UPROPERTY(Transient)
	TMap<UClass*, FLootLevelData> LevelDataByClass{};

	int32 CurrentLevel{};
	int32 EnemiesDestroyedThisLevel{};
};
