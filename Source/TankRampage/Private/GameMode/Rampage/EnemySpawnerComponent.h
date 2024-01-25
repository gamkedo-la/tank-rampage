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
	void DoSpawnTimeSlice();
	bool IsSpawnerStateValid() const;

	void CalculateEligibleSpawners(const APawn& PlayerPawn);
	float CalculateSpawnIntervalTime() const;

	std::optional<FEnemySpawnerData> GetCurrentSpawnerData() const;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	float EarliestSpawningGameTimeSeconds{ 5.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	float ClusterMatchScoreMultiplier{ 3.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	float MinSpawnInterval{ 1 / 30.0f };

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

	struct FCurrentSpawnerState
	{
		FEnemySpawnerData SpawnerData{};
		int32 EligibleSpawnersIndex{};

		void Reset();
	};

	FCurrentSpawnerState CurrentSpawnerState{};

	UPROPERTY(EditDefaultsOnly, Category = Data)
	TObjectPtr<UDataTable> EnemySpawnerDataTable{};

	TArray<FEnemySpawnerData> SpawnerDataByMinute;
	float SpawningOffsetTime{ -1.0f };

	FTimerHandle SpawnLoopTimer{};
	FTimerHandle SpawningTimer{};
};
