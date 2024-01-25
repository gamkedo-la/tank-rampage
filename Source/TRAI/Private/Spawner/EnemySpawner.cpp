// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner/EnemySpawner.h"

#include "Spawner/SpawnLocationComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRAILogging.h"
#include "VisualLogger/VisualLogger.h"
#include "Utils/RandUtils.h"

#include "Camera/CameraComponent.h"

#include <limits>
#include <optional>
#include <array>
#include <chrono>
#include <algorithm>

namespace
{
	std::optional<float> GetCameraFOVFor(const APawn& Pawn);
	bool IsInFOV(const APawn& Pawn, const FVector& SpawnReferenceLocation);
}

// Initialize the random number generator with a seed from current time
AEnemySpawner::AEnemySpawner() : Rng(RandUtils::GenerateSeed())
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Spawn Point"));

	FirstSpawnLocation = CreateDefaultSubobject<USpawnLocationComponent>(TEXT("Spawn Point 1"));
	FirstSpawnLocation->SetupAttachment(GetRootComponent());
}

bool AEnemySpawner::CanSpawnAnyFor(const APawn& PlayerPawn, float* OutScore) const
{
	if (SpawningTypes.IsEmpty() || SpawnLocations.IsEmpty())
	{
		return false;
	}

	const auto& PlayerLocation = GetActorLocation();
	const auto& SpawnReferenceLocation = GetSpawnReferenceLocation();

	const auto RefDistSq = FVector::DistSquared(PlayerLocation, SpawnReferenceLocation);

	// out of spawn range
	if (RefDistSq > FMath::Square(MaxDistance))
	{
		return false;
	}

	// Check min distance based on wheter in FOV of player
	const bool bInFOV = IsInFOV(PlayerPawn, SpawnReferenceLocation);
	const float ReferenceMinDistance = bInFOV ? MinimumDistanceFOV : MinimumDistanceNotFOV;

	if (RefDistSq < FMath::Square(ReferenceMinDistance))
	{
		return false;
	}

	if (NoSpawnZoneComponent && NoSpawnZoneComponent->IsOverlappingActor(&PlayerPawn))
	{
		return false;
	}

	if (OutScore)
	{
		const auto Dist = FMath::Sqrt(RefDistSq);
		float Score = (Dist - ReferenceMinDistance) * DistanceUnitMultiplier;
		if (bInFOV)
		{
			Score *= FOVScoreMultiplier;
		}

		*OutScore = Score;
	}

	return true;
}

int32 AEnemySpawner::Spawn(int32 DesiredCount, TArray<APawn*>* OutSpawned)
{
	const auto SpawnClass = SelectSpawnClass();
	if (!SpawnClass)
	{
		return 0;
	}

	auto World = GetWorld();
	check(World);

	DesiredCount = FMath::Min(DesiredCount, SpawnLocations.Num());

	if (DesiredCount == 0)
	{
		return 0;
	}

	std::array<int32, 1024> Indices;
	int32 LocationCount = SpawnLocations.Num();
	if (!ensureAlwaysMsgf(Indices.max_size() <= SpawnLocations.Num(), TEXT("%s: SpawnLocations.Num()=%d > %d"),
		*GetName(), SpawnLocations.Num(), Indices.max_size()))
	{
		LocationCount = Indices.max_size();
	}

	RandUtils::ShuffleIndices(Indices.begin(), Rng, LocationCount);
	int32 SpawnedCount{};

	for (int32 i = 0; i < LocationCount && SpawnedCount < DesiredCount; ++i)
	{
		FActorSpawnParameters Params;

		auto SpawnLocationActor = SpawnLocations[Indices[i]];
		if (!SpawnLocationActor)
		{
			continue;
		}
		const auto& SpawnLocation = SpawnLocationActor->GetComponentLocation();

		auto Spawned = World->SpawnActor<APawn>(SpawnClass, SpawnLocation, SpawnLocationActor->GetComponentRotation(), Params);
		if (Spawned)
		{
			UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s: Spawn - Spawned %s -> %s"), *GetName(), *SpawnClass->GetName(), *Spawned->GetName());
			UE_VLOG_LOCATION(this, LogTRAI, Log, SpawnLocation, 50.0f, FColor::Green, TEXT("Spawn %s"), *SpawnClass->GetName());

			++SpawnedCount;

			if (OutSpawned)
			{
				OutSpawned->Add(Spawned);
			}
		}
		else
		{
			UE_VLOG_UELOG(this, LogTRAI, Warning, TEXT("%s: Spawn - Failed to Spawn %s"), *GetName(), *SpawnClass->GetName());
			UE_VLOG_LOCATION(this, LogTRAI, Warning, SpawnLocation, 50.0f, FColor::Red, TEXT("Failed to spawn %s"), *SpawnClass->GetName());
		}
	}

	if (SpawnedCount > 0)
	{
		LastSpawnTime = World->GetTimeSeconds();
	}

	return SpawnedCount;
}

float AEnemySpawner::GetTimeSinceLastSpawn() const
{
	auto World = GetWorld();
	if (!World || LastSpawnTime < 0)
	{
		return std::numeric_limits<float>::max();
	}

	return World->GetTimeSeconds() - LastSpawnTime;
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s: BeginPlay - %d spawn locations found"), *GetName(), SpawnLocations.Num());
}

void AEnemySpawner::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	GetComponents(SpawnLocations);
}

void AEnemySpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AEnemySpawner::Initialize(UPrimitiveComponent* InOverlapComponent)
{
	UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s: Initialize - NoSpawnZoneComponent=%s;SpawningArea=%s"), *GetName(), *LoggingUtils::GetName(InOverlapComponent));

	if (!ensure(InOverlapComponent))
	{
		return;
	}

	NoSpawnZoneComponent = InOverlapComponent;
}

UClass* AEnemySpawner::SelectSpawnClass() const
{
	if (!ensureMsgf(!SpawningTypes.IsEmpty(), TEXT("%s: SelectSpawnClass - SpawningTypes is empty"), *GetName()))
	{
		return nullptr;
	}

	TArray<int32, TInlineAllocator<64>> ValidTypeIndices;
	for (int32 i = 0; i < SpawningTypes.Num(); ++i)
	{
		if (!CanSpawnEnemy.IsBound() || CanSpawnEnemy.Execute(SpawningTypes[i]))
		{
			ValidTypeIndices.Add(i);
		}
	}

	if (ValidTypeIndices.IsEmpty())
	{
		UE_VLOG_UELOG(this, LogTRAI, Display, TEXT("%s: SelectSpawnClass - could not find an available spawning type from the %d available"), *GetName(), SpawningTypes.Num());
		return nullptr;
	}

	const int32 SelectedValidIndex = FMath::RandRange(0, ValidTypeIndices.Num() - 1);
	check(SelectedValidIndex < ValidTypeIndices.Num());

	const int32 SelectedIndex = ValidTypeIndices[SelectedValidIndex];
	check(SelectedIndex >= 0 && SelectedIndex < SpawningTypes.Num());

	return SpawningTypes[SelectedIndex];
}

FVector AEnemySpawner::GetSpawnReferenceLocation() const
{
	if (SpawnLocations.IsEmpty())
	{
		return GetActorLocation();
	}

	FVector AverageLocation{ EForceInit::ForceInitToZero };

	for (auto SpawnLocation : SpawnLocations)
	{
		check(SpawnLocation);
		AverageLocation += SpawnLocation->GetComponentLocation();
	}

	AverageLocation /= SpawnLocations.Num();

	return AverageLocation;
}

namespace
{
	std::optional<float> GetCameraFOVFor(const APawn& Pawn)
	{
		auto CameraComponent = Pawn.FindComponentByClass<UCameraComponent>();
		if (!CameraComponent)
		{
			return {};
		}

		return CameraComponent->FieldOfView;
	}

	bool IsInFOV(const APawn& Pawn, const FVector& SpawnReferenceLocation)
	{
		auto FOVOpt = GetCameraFOVFor(Pawn);
		if (!FOVOpt)
		{
			return false;
		}

		const auto FOVHalfAngleRads = FMath::DegreesToRadians(*FOVOpt * 0.5f);

		const auto& ActorForwardVector = Pawn.GetActorForwardVector();
		const auto& ActorLocation = Pawn.GetActorLocation();

		const auto ToSpawnDirection = (SpawnReferenceLocation - ActorLocation).GetSafeNormal();

		const auto ToSpawnHalfAngleRads = FMath::Acos(ToSpawnDirection | ActorForwardVector);

		return ToSpawnHalfAngleRads <= FOVHalfAngleRads;
	}
}