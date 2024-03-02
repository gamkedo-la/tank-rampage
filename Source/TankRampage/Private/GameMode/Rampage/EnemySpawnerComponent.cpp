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

	AvailableSpawnerIndices.Reset(Spawners.Num());
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

	LastEligibleSpawnersSortTime = -1.0f;

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

	CalculatePossibleSpawners(*PlayerPawn);

	CurrentSpawnerState.NumTotalSpawners = AvailableSpawnerIndices.Num();

	if (AvailableSpawnerIndices.IsEmpty())
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: CalculateSpawningLoop - No available spawners"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return -1;
	}
	
	const auto [SpawnIntervalTime, SpawnCycles] = CalculateSpawnIntervalTimeAndCycles();

	CurrentSpawnerState.IntervalsRemaining = CurrentSpawnerState.TotalIntervals = SpawnCycles;

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
	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: DoSpawnTimeSlice interval %d/%d running..."),
		*LoggingUtils::GetName(GetOwner()), *GetName(), CurrentSpawnerState.TotalIntervals - CurrentSpawnerState.IntervalsRemaining + 1, CurrentSpawnerState.TotalIntervals);

	auto PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Display, TEXT("%s-%s: DoSpawnTimeSlice - PlayerPawn is NULL - disabling spawns"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		ClearSpawningTimer();
		return;
	}

	if (!CalculateEligibleSpawnersAsNeeded(*PlayerPawn))
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Display, TEXT("%s-%s: DoSpawnTimeSlice - No eligible spawners are available"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		ClearSpawningTimer();
		return;
	}

	int32& SpawnerIndex = CurrentSpawnerState.EligibleSpawnersIndex;
	int32& TotalOverallSpawns = CurrentSpawnerState.TotalSpawned;
	int32 IterationCount{}, SpawnCountCurrentLoop{}, TotalSliceSpawns{};
	const int32 DesiredCount = CurrentSpawnerState.GetDesiredSpawnCount();
	const AActor* LookAtActor = CurrentSpawnerState.LookAtActor.Get();

	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: DoSpawnTimeSlice - Begin: %d desired with %d/%d spawned already"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), DesiredCount, TotalOverallSpawns, CurrentSpawnerState.SpawnerData.SpawnCount);

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

		if (SpawnerIndex >= EligibleSpawners.Num() && (IterationCount < EligibleSpawners.Num() || SpawnCountCurrentLoop > 0))
		{
			// Refresh the eligible spawners
			CalculateEligibleSpawners(*PlayerPawn);
			SpawnCountCurrentLoop = 0;
		}
	};

	while (SpawnerIndex < EligibleSpawners.Num())
	{
		auto& SpawnerState = EligibleSpawners[SpawnerIndex];
		auto EnemySpawner = Spawners[SpawnerState.Index];
		if (!IsValid(EnemySpawner))
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
	EligibleSpawners.Reset();

	auto World = GetWorld();
	check(World);

	const auto DesiredClusterSize = CurrentSpawnerState.SpawnerData.SpawnClusterSize;

	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: CalculateEligibleSpawners - AvailableSpawnerIndices = %d / %d; DesiredClusterSize=%d"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), AvailableSpawnerIndices.Num(), Spawners.Num(), DesiredClusterSize);

	for (auto SpawnerIndexIt = AvailableSpawnerIndices.CreateIterator(); SpawnerIndexIt; ++SpawnerIndexIt)
	{
		const int32 SpawnerIndex = *SpawnerIndexIt;

		auto Spawner = Spawners[SpawnerIndex];

		if (!IsValid(Spawner))
		{
			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: CalculateSpawningLoop - Removed de-allocated spawner"),
				*LoggingUtils::GetName(GetOwner()), *GetName());
			SpawnerIndexIt.RemoveCurrent();
			continue;
		}

		float SpawnerScore;
		if (!Spawner->CanSpawnAnyFor(PlayerPawn, &SpawnerScore))
		{
			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Verbose, TEXT("%s-%s: CalculateSpawningLoop - Spawner %s cannot spawn any"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *Spawner->GetName());

			continue;
		}

		// Lower score is better
		if (Spawner->GetMaxSpawnCount() >= DesiredClusterSize)
		{
			SpawnerScore /= ClusterMatchScoreMultiplier;
		}

		EligibleSpawners.Add(FSpawnerMetadata
		{
			.Index = SpawnerIndex,
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

	CurrentSpawnerState.EligibleSpawnersIndex = 0;
	CurrentSpawnerState.NumEligibleSpawners = EligibleSpawners.Num();

	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: CalculateEligibleSpawners - NumEligibleSpawners=%d"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), CurrentSpawnerState.NumEligibleSpawners);

	LastEligibleSpawnersSortTime = World->GetTimeSeconds();
}

void UEnemySpawnerComponent::CalculatePossibleSpawners(const APawn& PlayerPawn)
{
	AvailableSpawnerIndices.Reset();

	// calculation interval is every minute
	const float ConsiderationRadiusSq = FMath::Square(PlayerMaxSpeed * 60.0f);

	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Verbose, TEXT("%s-%s: CalculatePossibleSpawners - ConsiderationRadius=%fm"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), PlayerMaxSpeed * 60.0 / 100);

	const auto StartingSpawnerCount = Spawners.Num();

	for (auto SpawnerIt = Spawners.CreateIterator(); SpawnerIt; ++SpawnerIt)
	{
		auto Spawner = *SpawnerIt;
		if (!IsValid(Spawner))
		{
			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: CalculatePossibleSpawners - Removed de-allocated spawner"),
				*LoggingUtils::GetName(GetOwner()), *GetName());
			SpawnerIt.RemoveCurrent();
			continue;
		}

		if (Spawner->ShouldBeConsideredForSpawning(PlayerPawn, ConsiderationRadiusSq))
		{
			AvailableSpawnerIndices.Add(SpawnerIt.GetIndex());
		}
	}

	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: CalculatePossibleSpawners - %d/%d available spawners: %d were removed"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), AvailableSpawnerIndices.Num(), Spawners.Num(), StartingSpawnerCount - Spawners.Num());
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

bool UEnemySpawnerComponent::IsDueForSpawnerPrioritization() const
{
	auto World = GetWorld();
	check(World);

	return LastEligibleSpawnersSortTime <= 0 || World->GetTimeSeconds() - LastEligibleSpawnersSortTime > MinRecalculationTimeSeconds;
}

bool UEnemySpawnerComponent::CalculateEligibleSpawnersAsNeeded(const APawn& PlayerPawn)
{
	auto World = GetWorld();
	check(World);

	if (CurrentSpawnerState.AnySpawnersAreAvailable() && !IsDueForSpawnerPrioritization())
	{
		return true;
	}

	if (AvailableSpawnerIndices.IsEmpty())
	{
		return false;
	}

	CalculateEligibleSpawners(PlayerPawn);

	return CurrentSpawnerState.AnySpawnersAreAvailable();
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
	// NumEligibleSpawners may be low but as player moves new spawners may become eligible so balance this with total num spawners in the loop
	const int32 SpawnersRemaining = FMath::Min(NumTotalSpawners, 
		(NumEligibleSpawners - EligibleSpawnersIndex) + NumEligibleSpawners * (FMath::Max(IntervalsRemaining - 1, 0)));

	if (SpawnersRemaining > 0)
	{
		DesiredSpawnDistribution = FMath::Max(DesiredSpawnDistribution, FMath::CeilToInt32(Remaining / (static_cast<float>(SpawnersRemaining) * DesiredSpawnDistribution)));
	}

	return FMath::Min(DesiredSpawnDistribution, Remaining);
}

bool UEnemySpawnerComponent::FCurrentSpawnerState::AnySpawnersAreAvailable() const
{
	return EligibleSpawnersIndex < NumEligibleSpawners;
}

int32 UEnemySpawnerComponent::FCurrentSpawnerState::SpawnsRemaining() const
{
	return SpawnerData.SpawnCount - TotalSpawned;
}

bool UEnemySpawnerComponent::FCurrentSpawnerState::HasSpawnsRemaining() const
{
	return SpawnsRemaining() > 0;
}
