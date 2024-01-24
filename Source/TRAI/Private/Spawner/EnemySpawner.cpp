// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner/EnemySpawner.h"

#include "Spawner/SpawnLocationComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRAILogging.h"
#include "VisualLogger/VisualLogger.h"

#include "Camera/CameraComponent.h"

#include <limits>
#include <optional>

namespace
{
	std::optional<float> GetCameraFOVFor(const APawn& Pawn);
	bool IsInFOV(const APawn& Pawn, const FVector& SpawnReferenceLocation);
}
AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Spawn Point"));

	FirstSpawnLocation = CreateDefaultSubobject<USpawnLocationComponent>(TEXT("Spawn Point 1"));
	FirstSpawnLocation->SetupAttachment(GetRootComponent());
}

bool AEnemySpawner::CanSpawnAnyFor(const APawn& PlayerPawn) const
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
	const float ReferenceMinDistance = IsInFOV(PlayerPawn, SpawnReferenceLocation) ? MinimumDistanceFOV : MinimumDistanceNotFOV;

	if (RefDistSq < FMath::Square(ReferenceMinDistance))
	{
		return false;
	}

	if (NoSpawnZoneComponent && NoSpawnZoneComponent->IsOverlappingActor(&PlayerPawn))
	{
		return false;
	}

	return true;
}

int32 AEnemySpawner::Spawn(int32 DesiredCount, TArray<APawn*>& OutSpawned)
{
	// TODO: Implement DesiredCount logic based on spawn points
	const auto SpawnClass = SelectSpawnClass();
	if (!SpawnClass)
	{
		return 0;
	}

	auto World = GetWorld();
	check(World);

	FActorSpawnParameters Params;

	auto Spawned = World->SpawnActor<APawn>(SpawnClass, GetActorLocation(), FRotator::ZeroRotator, Params);
	if (Spawned)
	{
		UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s: Spawn - Spawned %s -> %s"), *GetName(), *SpawnClass->GetName(), *Spawned->GetName());
		UE_VLOG_LOCATION(this, LogTRAI, Log, GetActorLocation(), 50.0f, FColor::Green, TEXT("Spawn %s"), *SpawnClass->GetName());
		LastSpawnTime = World->GetTimeSeconds();
	}
	else
	{
		UE_VLOG_UELOG(this, LogTRAI, Warning, TEXT("%s: Spawn - Failed to Spawn %s"), *GetName(), *SpawnClass->GetName());
		UE_VLOG_LOCATION(this, LogTRAI, Warning, GetActorLocation(), 50.0f, FColor::Red, TEXT("Failed to spawn %s"), *SpawnClass->GetName());

		OutSpawned.Add(Spawned);
		return 0;
	}

	return 1;
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