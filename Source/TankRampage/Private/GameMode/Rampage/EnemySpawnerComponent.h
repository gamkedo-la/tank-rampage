// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemySpawnerData.h"
#include <optional>

#include "EnemySpawnerComponent.generated.h"

class AEnemySpawner;
class UDataTable;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UEnemySpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UEnemySpawnerComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
	void InitSpawners();
	void InitData();

	void InitSpawningSchedule();
	void ScheduleSpawning();
	float CalculateSpawningLoop();

	void ClearAllTimers();
	void ClearSpawningTimer();

	void DoSpawnTimeSlice();
	bool IsSpawnerStateValid() const;
	bool TryRefreshSpawnersAndRescheduleIfInvalid();

	void CalculateEligibleSpawners(const APawn& PlayerPawn);
	std::pair<float,int32> CalculateSpawnIntervalTimeAndCycles() const;

	std::optional<FEnemySpawnerData> GetCurrentSpawnerData() const;

	bool TryReserveAdditionalAvailableSpawner();

private:

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	float EarliestSpawningGameTimeSeconds{ 5.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	float ClusterMatchScoreMultiplier{ 3.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	float MinSpawnInterval{ 1 / 30.0f };

	/**
	* Set to value > 0 to refresh the spawner array for example if the spawners are spatially loaded.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	int32 SpawnerRefreshMinutes{ -1 };

	UPROPERTY(Transient)
	TArray<AEnemySpawner*> Spawners;

	struct FSpawnerMetadata
	{
		int32 Index;
		uint16 SpawnCount;
		uint16 VisitCount;
		float Score;
	};

	TArray<FSpawnerMetadata> EligibleSpawners;
	TSet<int32> AvailableSpawnerIndices;

	struct FCurrentSpawnerState
	{
		FEnemySpawnerData SpawnerData{};
		int32 EligibleSpawnersIndex{};
		int32 TotalSpawned{};
		int32 IntervalsRemaining{};
		int32 NumSpawners{};

		TWeakObjectPtr<const AActor> LookAtActor{};

		void Reset();
		int32 SpawnsRemaining() const;
		bool HasSpawnsRemaining() const;
		int32 GetDesiredSpawnCount() const;
	};

	FCurrentSpawnerState CurrentSpawnerState{};

	UPROPERTY(EditDefaultsOnly, Category = Data)
	TObjectPtr<UDataTable> EnemySpawnerDataTable{};

	TArray<FEnemySpawnerData> SpawnerDataByMinute;
	float SpawningOffsetTime{ -1.0f };
	int32 SpawnerRefreshTicks{};

	FTimerHandle SpawnLoopTimer{};
	FTimerHandle SpawningTimer{};
};
