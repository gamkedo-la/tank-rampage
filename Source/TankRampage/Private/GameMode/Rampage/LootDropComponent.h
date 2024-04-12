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
	UFUNCTION()
	void OnTankDestroyed(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith);

	UFUNCTION()
	void OnXPLevelUp(int32 NewLevel);

	void SpawnLoot(const AController* Owner, const FVector& BaseSpawnLocation);

	const ABasePickup* SpawnLoot(const AController* Owner, const FVector& SpawnLocation, UClass* PickupClass) const;
	FVector GetSpawnLocation(const TSubclassOf<ABasePickup>& PickupClass, const FVector& BaseLocation) const;

	FVector GetSpawnOffsetLocation(const TSubclassOf<ABasePickup>& PickupClass, const FVector& BaseLocation) const;
	TOptional<FBox> GetPickupBounds(const TSubclassOf<ABasePickup>& PickupClass) const;
	FVector GroundSpawnLocation(const TSubclassOf<ABasePickup>& PickupClass, const FVector& Location) const;

	bool ShouldSpawnLootClass(const FLootConfig& LootConfig) const;

	bool ValidateConfig();
	bool ValidateCurveTable(UCurveTable* CurveTable) const;

	void InitializeLevelData(int32 Level);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TArray<FLootConfig> LootConfigs{};

	UPROPERTY(EditDefaultsOnly)
	float SpawnRadius{ 300.0f };

	UPROPERTY(Transient)
	TMap<UClass*, FLootLevelData> LevelDataByClass{};

	int32 CurrentLevel{};
	int32 EnemiesDestroyedThisLevel{};
};
