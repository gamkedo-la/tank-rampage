// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/EnemySpawnerComponent.h"

#include "Spawner/EnemySpawner.h"
#include "Kismet/GameplayStatics.h"
#include "TankRampageLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

UEnemySpawnerComponent::UEnemySpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemySpawnerComponent::BeginPlay()
{
	Super::BeginPlay();

	InitData();
	InitSpawners();

	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: BeginPlay - Found %d enemy spawners and %d enemy spawn minute configs"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), Spawners.Num(), SpawnerDataByMinute.Num());

	InitSpawningSchedule();
}

void UEnemySpawnerComponent::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ClearAllTimers();
}

void UEnemySpawnerComponent::InitSpawners()
{
	auto World = GetWorld();
	check(World);

	Spawners.Reset();

	for (TObjectIterator<AEnemySpawner> It; It; ++It)
	{
		if (World != It->GetWorld())
		{
			continue;
		}

		Spawners.Add(*It);
	}

	CurrentSpawnerState.Reset();
	EligibleSpawners.Reset(Spawners.Num());

	AvailableSpawnerIndices.Reserve(Spawners.Num());
	AvailableSpawnerIndices.Reset();
}

void UEnemySpawnerComponent::InitData()
{
	SpawnerDataByMinute = EnemySpawnerDataParser::ReadAll(EnemySpawnerDataTable);
}

void UEnemySpawnerComponent::InitSpawningSchedule()
{
	auto World = GetWorld();
	check(World);

	if (!IsSpawnerStateValid())
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Error, TEXT("%s-%s: InitSpawningSchedule - Spawning state is invalid - no spawning will occur!"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return;
	}

	float SpawnDelayTime{};

	if (SpawningOffsetTime < 0)
	{
		// Only delay on first execution since if this gets called again we want it to schedule immediately
		SpawnDelayTime = EarliestSpawningGameTimeSeconds;
		SpawningOffsetTime = World->GetTimeSeconds() + EarliestSpawningGameTimeSeconds;
	}

	World->GetTimerManager().SetTimer(SpawnLoopTimer, this, &ThisClass::ScheduleSpawning, 60.0f, true, SpawnDelayTime);

	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: InitSpawningSchedule - Schedule spawning loop every minute"),
		*LoggingUtils::GetName(GetOwner()), *GetName());
}

void UEnemySpawnerComponent::ScheduleSpawning()
{
	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: ScheduleSpawning running..."),
		*LoggingUtils::GetName(GetOwner()), *GetName());

	auto World = GetWorld();
	check(World);

	auto& TimerManager = World->GetTimerManager();

	// Clear any existing spawning happening
	TimerManager.ClearTimer(SpawningTimer);

	const auto Interval = CalculateSpawningLoop();

	if (Interval < 0)
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Warning, TEXT("%s-%s: ScheduleSpawning - Unable to schedule next minute spawn loop"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		
		// Check spawning config is now also invalid
		if (!TryRefreshSpawnersAndRescheduleIfInvalid())
		{
			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Warning, TEXT("%s-%s: ScheduleSpawning - Spawning state became invalid - no additional spawning will occur!"),
				*LoggingUtils::GetName(GetOwner()), *GetName());
			TimerManager.ClearTimer(SpawnLoopTimer);
		}
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: ScheduleSpawning - Interval=%fs; TotalSliceSpawns=%d; DesiredClusterSize=%d"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), Interval, CurrentSpawnerState.SpawnerData.SpawnCount, CurrentSpawnerState.SpawnerData.SpawnClusterSize
	);

	TimerManager.SetTimer(SpawningTimer, this, &ThisClass::DoSpawnTimeSlice, Interval, true, 0.0f);
}

float UEnemySpawnerComponent::CalculateSpawningLoop()
{
	EligibleSpawners.Reset();
	CurrentSpawnerState.Reset();
	AvailableSpawnerIndices.Reset();

	const auto SpawnConfig = GetCurrentSpawnerData();
	if (!SpawnConfig)
	{
		return -1;
	}

	auto PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: CalculateSpawningLoop - No player pawn"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return -1;
	}

	CurrentSpawnerState.SpawnerData = *SpawnConfig;
	CurrentSpawnerState.LookAtActor = PlayerPawn;

	CalculateEligibleSpawners(*PlayerPawn);

	if (EligibleSpawners.IsEmpty())
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: CalculateSpawningLoop - No eligible spawners"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return -1;
	}
	
	const auto [SpawnIntervalTime, SpawnCycles] = CalculateSpawnIntervalTimeAndCycles();

	CurrentSpawnerState.IntervalsRemaining = SpawnCycles;
	CurrentSpawnerState.NumSpawners = EligibleSpawners.Num();

	return SpawnIntervalTime;
}

void UEnemySpawnerComponent::ClearAllTimers()
{
	if (auto World = GetWorld(); World)
	{
		auto& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(SpawnLoopTimer);
		TimerManager.ClearTimer(SpawningTimer);
	}
}

void UEnemySpawnerComponent::ClearSpawningTimer()
{
	if (auto World = GetWorld(); World)
	{
		auto& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(SpawningTimer);
	}
}

void UEnemySpawnerComponent::DoSpawnTimeSlice()
{
	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: DoSpawnTimeSlice running..."),
		*LoggingUtils::GetName(GetOwner()), *GetName());

	auto PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Display, TEXT("%s-%s: DoSpawnTimeSlice - PlayerPawn is NULL - disabling spawns"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		ClearSpawningTimer();
		return;
	}

	int32& SpawnerIndex = CurrentSpawnerState.EligibleSpawnersIndex;
	int32& TotalOverallSpawns = CurrentSpawnerState.TotalSpawned;
	int32 IterationCount{}, SpawnCountCurrentLoop{}, TotalSliceSpawns{};
	const int32 DesiredCount = CurrentSpawnerState.GetDesiredSpawnCount();
	const AActor* LookAtActor = CurrentSpawnerState.LookAtActor.Get();

	--CurrentSpawnerState.IntervalsRemaining;

	// Should happen at end of previous loop
	if (!ensureMsgf(DesiredCount > 0, TEXT("DesiredCount <= 0 : SpawnClusterSize=%d; TotalSpawnCount=%d; TotalSpawnsRequested=%d"),
		DesiredCount, CurrentSpawnerState.SpawnerData.SpawnClusterSize, CurrentSpawnerState.TotalSpawned, CurrentSpawnerState.SpawnerData.SpawnCount))
	{
		ClearSpawningTimer();
		return;
	}

	auto IncrementSpawner = [&, this]()
	{
		++IterationCount;
		++SpawnerIndex;

		if (SpawnerIndex >= EligibleSpawners.Num())
		{
			// attempt to reserve an additional spawner that wasn't selected initially
			TryReserveAdditionalAvailableSpawner();
		}
		if (SpawnerIndex >= EligibleSpawners.Num() && (IterationCount < EligibleSpawners.Num() || SpawnCountCurrentLoop > 0))
		{
			SpawnerIndex = 0;
			SpawnCountCurrentLoop = 0;
		}
	};

	while (SpawnerIndex < EligibleSpawners.Num())
	{
		auto& SpawnerState = EligibleSpawners[SpawnerIndex];
		auto EnemySpawner = Spawners[SpawnerState.Index];
		if (!EnemySpawner)
		{
			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: DoSpawnTimeSlice - Skipped de-allocated spawner"),
				*LoggingUtils::GetName(GetOwner()), *GetName());
			IncrementSpawner();
			continue;
		}

		// Recheck spawn eligibility since player has been moving since this was calculated at the start of the minute
		check(PlayerPawn);
		if (!EnemySpawner->CanSpawnAnyFor(*PlayerPawn))
		{
			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: DoSpawnTimeSlice - Skipped spawner %s that is no longer relevant for player"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *EnemySpawner->GetName());
			IncrementSpawner();
			continue;
		}

		const int32 RequestedSpawns = FMath::Min(DesiredCount - TotalSliceSpawns, EnemySpawner->GetMaxSpawnCount());
		checkf(RequestedSpawns >= 0, TEXT("RequestedSpawns=%d; DesiredCount=%d; TotalSliceSpawns=%d; MaxSpawnCount=%d"),
			RequestedSpawns, DesiredCount, TotalSliceSpawns, EnemySpawner->GetMaxSpawnCount());

		const int32 ActualSpawned = EnemySpawner->Spawn(RequestedSpawns, LookAtActor);
		++SpawnerState.VisitCount;
		SpawnerState.SpawnCount += ActualSpawned;

		SpawnCountCurrentLoop += ActualSpawned;
		TotalSliceSpawns += ActualSpawned;
		TotalOverallSpawns += ActualSpawned;

		IncrementSpawner();

		if (TotalSliceSpawns == DesiredCount)
		{
			break;
		}
	}

	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: DoSpawnTimeSlice - Spawned %d/%d in %d spawners : %d/%d of current loop complete"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), TotalSliceSpawns, DesiredCount, IterationCount, TotalOverallSpawns, CurrentSpawnerState.SpawnerData.SpawnCount);

	if (!CurrentSpawnerState.HasSpawnsRemaining())
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: DoSpawnTimeSlice - Current loop complete. Canceling future spawn schedules"),
			*LoggingUtils::GetName(GetOwner()), *GetName());

		ClearSpawningTimer();
	}
}

bool UEnemySpawnerComponent::IsSpawnerStateValid() const
{
	return !Spawners.IsEmpty() && !SpawnerDataByMinute.IsEmpty();
}

bool UEnemySpawnerComponent::TryRefreshSpawnersAndRescheduleIfInvalid()
{
	if (IsSpawnerStateValid())
	{
		return true;
	}

	InitSpawners();

	if (IsSpawnerStateValid())
	{
		ScheduleSpawning();
		return true;
	}

	return false;
}

void UEnemySpawnerComponent::CalculateEligibleSpawners(const APawn& PlayerPawn)
{
	const auto DesiredClusterSize = CurrentSpawnerState.SpawnerData.SpawnClusterSize;

	for (auto SpawnerIt = Spawners.CreateIterator(); SpawnerIt; ++SpawnerIt)
	{
		auto Spawner = *SpawnerIt;
		if (!IsValid(Spawner))
		{
			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: CalculateSpawningLoop - Removed de-allocated spawner"),
				*LoggingUtils::GetName(GetOwner()), *GetName());
			SpawnerIt.RemoveCurrent();
			continue;
		}

		float SpawnerScore;
		if (!Spawner->CanSpawnAnyFor(PlayerPawn, &SpawnerScore))
		{
			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Verbose, TEXT("%s-%s: CalculateSpawningLoop - Spawner %s cannot spawn any"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *Spawner->GetName());

			// add as a possible candidate for a retry later
			AvailableSpawnerIndices.Add(SpawnerIt.GetIndex());

			continue;
		}

		// Lower score is better
		if (Spawner->GetMaxSpawnCount() >= DesiredClusterSize)
		{
			SpawnerScore /= ClusterMatchScoreMultiplier;
		}

		EligibleSpawners.Add(FSpawnerMetadata
		{
			.Index = SpawnerIt.GetIndex(),
			.SpawnCount = 0,
			.VisitCount = 0,
			.Score = SpawnerScore
		});
	}

	// Find best spawners
	EligibleSpawners.Sort([](const auto& First, const auto& Second)
	{
		return First.Score < Second.Score;
	});
}

std::pair<float, int32> UEnemySpawnerComponent::CalculateSpawnIntervalTimeAndCycles() const
{
	const float TotalSpawnCount = CurrentSpawnerState.SpawnerData.SpawnCount;
	const float DesiredClusterSize = CurrentSpawnerState.SpawnerData.SpawnClusterSize;

	const float DesiredSpawnCycles = FMath::CeilToFloat(TotalSpawnCount / DesiredClusterSize);
	const float SpawnInterval = FMath::Max(MinSpawnInterval, 60.0f / DesiredSpawnCycles);

	const int32 ActualSpawnCycles = FMath::FloorToInt32(60.0f / SpawnInterval);

	return { SpawnInterval, ActualSpawnCycles };
}

std::optional<FEnemySpawnerData> UEnemySpawnerComponent::GetCurrentSpawnerData() const
{
	auto World = GetWorld();
	check(World);

	const int32 CurrentMinute = FMath::FloorToInt32((World->GetTimeSeconds() - SpawningOffsetTime) / 60);
	if (!ensureAlwaysMsgf(CurrentMinute >= 0, TEXT("CurrentMinute=%f; WorldTimeSeconds=%f; SpawningOffsetTime=%f"), CurrentMinute, World->GetTimeSeconds(), SpawningOffsetTime))
	{
		return {};
	}

	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Verbose, TEXT("%s-%s: GetCurrentSpawnerData - CurrentMinute = %d"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), CurrentMinute);

	if (SpawnerDataByMinute.IsEmpty())
	{
		return {};
	}

	const int32 Index = FMath::Min(CurrentMinute, SpawnerDataByMinute.Num());

	return SpawnerDataByMinute[Index];
}

bool UEnemySpawnerComponent::TryReserveAdditionalAvailableSpawner()
{
	if (AvailableSpawnerIndices.IsEmpty())
	{
		return false;
	}

	auto It = AvailableSpawnerIndices.CreateIterator();

	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: TryReserveAdditionalAvailableSpawner - Spawner=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(Spawners[*It]));

	EligibleSpawners.Add(FSpawnerMetadata
	{
		.Index = *It,
		.SpawnCount = 0,
		.VisitCount = 0,
		.Score = std::numeric_limits<float>::max() // score uncalculated
	});

	It.RemoveCurrent();

	++CurrentSpawnerState.NumSpawners;

	return true;
}

void UEnemySpawnerComponent::FCurrentSpawnerState::Reset()
{
	*this = FCurrentSpawnerState{};
}

int32 UEnemySpawnerComponent::FCurrentSpawnerState::GetDesiredSpawnCount() const
{
	// make sure based on remaining intervals that we distribute correctly
	const int32 Remaining = SpawnsRemaining();

	if (Remaining <= 0)
	{
		return 0;
	}

	int32 DesiredSpawnDistribution{};
	if (IntervalsRemaining > 0)
	{
		DesiredSpawnDistribution = FMath::CeilToInt32(Remaining / static_cast<float>(IntervalsRemaining));
	}
	DesiredSpawnDistribution = FMath::Max(DesiredSpawnDistribution, SpawnerData.SpawnClusterSize);

	// This is called on the active spawner index so no need to -1 it
	const int32 SpawnersRemaining = NumSpawners - EligibleSpawnersIndex;
	if (SpawnersRemaining > 0)
	{
		DesiredSpawnDistribution = FMath::Max(DesiredSpawnDistribution, FMath::CeilToInt32(Remaining / (static_cast<float>(SpawnersRemaining) * DesiredSpawnDistribution)));
	}

	return FMath::Min(DesiredSpawnDistribution, Remaining);
}

int32 UEnemySpawnerComponent::FCurrentSpawnerState::SpawnsRemaining() const
{
	return SpawnerData.SpawnCount - TotalSpawned;
}

bool UEnemySpawnerComponent::FCurrentSpawnerState::HasSpawnsRemaining() const
{
	return SpawnsRemaining() > 0;
}
