// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner/EnemySpawner.h"

#include "Spawner/SpawnLocationComponent.h"

#include "Utils/CollisionUtils.h"

#include "Logging/LoggingUtils.h"
#include "TRAILogging.h"
#include "VisualLogger/VisualLogger.h"
#include "Utils/RandUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

#include "Subsystems/TankEventsSubsystem.h"

#include <limits>
#include <optional>
#include <array>
#include <chrono>
#include <algorithm>

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnemySpawner)

namespace
{
	struct FCameraFOVResult
	{
		FVector Location;
		float FOV;
	};

	std::optional<FCameraFOVResult> GetCameraFOVFor(const APawn& Pawn);
	bool IsInFOV(const APawn& Pawn, const FVector& SpawnReferenceLocation);

	FRotator GetSpawnActorRotation(const USceneComponent& SpawnLocationComponent, const AActor* LookAtActor);
}

// Initialize the random number generator with a seed from current time
AEnemySpawner::AEnemySpawner() : Rng(RandUtils::GenerateSeed())
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Spawn Point"));

	FirstSpawnLocation = CreateDefaultSubobject<USpawnLocationComponent>(TEXT("Spawn Point 1"));
	FirstSpawnLocation->SetupAttachment(GetRootComponent());
}

bool AEnemySpawner::ShouldBeConsideredForSpawning(const APawn& PlayerPawn, float ConsiderationRadiusSq) const
{
	// ignore cooldown 
	if (SpawningTypes.IsEmpty() || SpawnLocations.IsEmpty())
	{
		return false;
	}

	const auto& PlayerLocation = PlayerPawn.GetActorLocation();
	const auto& SpawnReferenceLocation = GetActorLocation();

	const auto RefDistSq = FVector::DistSquared(PlayerLocation, SpawnReferenceLocation);

	return RefDistSq <= ConsiderationRadiusSq + FMath::Square(MaxDistance);
}

bool AEnemySpawner::CanSpawnAnyFor(const APawn& PlayerPawn, float* OutScore) const
{
	if (SpawningTypes.IsEmpty() || SpawnLocations.IsEmpty() || IsCoolingDown())
	{
		return false;
	}

	const auto& PlayerLocation = PlayerPawn.GetActorLocation();
	const auto& SpawnReferenceLocation = GetSpawnReferenceLocation();

	const auto RefDistSq = FVector::DistSquared(PlayerLocation, SpawnReferenceLocation);

	// out of spawn range
	if (RefDistSq > FMath::Square(MaxDistance))
	{
		UE_VLOG_LOCATION(this, LogTRAI, Verbose, SpawnReferenceLocation, 50.0f, FColor::Orange, TEXT("No Spawn - Out of Range: Dist=%fm > MaxDist=%fm"),
			FMath::Sqrt(RefDistSq) / 100, MaxDistance / 100);
		return false;
	}

	// Check if we are already inside the no-spawn zone and quickly return false if so
	if (NoSpawnZoneComponent && NoSpawnZoneComponent->IsOverlappingActor(&PlayerPawn))
	{
		UE_VLOG_LOCATION(this, LogTRAI, Verbose, SpawnReferenceLocation, 50.0f, FColor::Orange, TEXT("No Spawn - Overlapping no spawn zone"));

		return false;
	}

	// Check min distance based on whether in FOV of player
	const bool bInFOV = IsInFOV(PlayerPawn, SpawnReferenceLocation);
	const float ReferenceMinDistance = bInFOV ? MinimumDistanceFOV : MinimumDistanceNotFOV;

	if (RefDistSq < FMath::Square(ReferenceMinDistance))
	{
		UE_VLOG_LOCATION(this, LogTRAI, Verbose, SpawnReferenceLocation, 50.0f, FColor::Orange, TEXT("No Spawn - bInFov=%s; Dist=%fm < MinDist=%fm"),
			LoggingUtils::GetBoolString(bInFOV), FMath::Sqrt(RefDistSq) / 100, ReferenceMinDistance / 100);
		return false;
	}

	if (OutScore)
	{
		*OutScore = CalculateSpawningScore(FMath::Sqrt(RefDistSq), ReferenceMinDistance, bInFOV);
	}

	UE_VLOG_LOCATION(this, LogTRAI, Verbose, SpawnReferenceLocation, 50.0f, FColor::Green, TEXT("Spawn Eligible"));

	return true;
}

float AEnemySpawner::CalculateSpawningScore(float DistanceFromPlayer, float MinDistance, bool bInFOV) const
{
	const float IdealDistance = FMath::Lerp(MinDistance, MaxDistance, IdealSpawnDistanceRatio);

	const float DistanceDeviation = FMath::Abs(DistanceFromPlayer - IdealDistance);
	float Score = DistanceDeviation * DistanceUnitMultiplier;

	if (bInFOV)
	{
		// Lower score is better so divide by the multiplier which is >= 1
		Score /= FOVScoreMultiplier;
	}

	return Score;
}

int32 AEnemySpawner::Spawn(int32 InDesiredCount, const AActor* LookAtActor, TArray<APawn*>* OutSpawned)
{
	UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s: Spawn - DesiredCount=%d; LookAtActor=%s; OutSpawned=[%s]; PossibleSpawnLocations=%d"),
		*GetName(), InDesiredCount, *LoggingUtils::GetName(LookAtActor), OutSpawned ? TEXT("NOT NULL") : TEXT("NULL"), SpawnLocations.Num());

	const auto SpawnClass = SelectSpawnClass();
	if (!SpawnClass)
	{
		return 0;
	}

	auto World = GetWorld();
	check(World);

	const int32 DesiredCount = FMath::Min(InDesiredCount, SpawnLocations.Num());

	if (DesiredCount <= 0)
	{
		UE_VLOG_UELOG(this, LogTRAI, Warning, TEXT("%s: Spawn - Failed to spawn as Min(DesiredCount=%d, SpawnLocationsNum=%d) = %d"),
			*GetName(), InDesiredCount, SpawnLocations.Num(), DesiredCount);

		return 0;
	}

	std::array<int32, 1024> Indices;
	int32 LocationCount = SpawnLocations.Num();
	if (!ensureAlwaysMsgf(SpawnLocations.Num() <= Indices.max_size(), TEXT("%s: SpawnLocations.Num()=%d > %d"),
		*GetName(), SpawnLocations.Num(), Indices.max_size()))
	{
		LocationCount = Indices.max_size();
	}

	RandUtils::ShuffleIndices(Indices.begin(), Rng, LocationCount);
	int32 SpawnedCount{};

	TArray<AActor*, TInlineAllocator<32>> AlreadySpawned;

	for (int32 i = 0; i < LocationCount && SpawnedCount < DesiredCount; ++i)
	{
		auto SpawnLocationActor = SpawnLocations[Indices[i]];
		if (!SpawnLocationActor)
		{
			continue;
		}

		const FTransform SpawnTransform(
			GetSpawnActorRotation(*SpawnLocationActor, LookAtActor),
			SpawnLocationActor->GetComponentLocation()
		);

		if (IsSpawnPointObstructed(SpawnClass, SpawnTransform, AlreadySpawned))
		{
			continue;
		}

		auto Spawned = World->SpawnActorDeferred<APawn>(SpawnClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
		if (Spawned)
		{
			UGameplayStatics::FinishSpawningActor(Spawned, SpawnTransform);

			UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s: Spawn - Spawned %s -> %s"), *GetName(), *SpawnClass->GetName(), *Spawned->GetName());
			UE_VLOG_LOCATION(this, LogTRAI, Log, SpawnTransform.GetLocation(), 50.0f, FColor::Green, TEXT("Spawn %s"), *SpawnClass->GetName());

			if (auto TankEventsSubsystem = World->GetSubsystem<UTankEventsSubsystem>(); ensure(TankEventsSubsystem))
			{
				TankEventsSubsystem->OnEnemySpawned.Broadcast(Spawned);
			}

			++SpawnedCount;

			AlreadySpawned.Add(Spawned);

			if (OutSpawned)
			{
				OutSpawned->Add(Spawned);
			}
		}
		else
		{
			UE_VLOG_UELOG(this, LogTRAI, Warning, TEXT("%s: Spawn - Failed to Spawn %s"), *GetName(), *SpawnClass->GetName());
			UE_VLOG_LOCATION(this, LogTRAI, Warning, SpawnTransform.GetLocation(), 50.0f, FColor::Red, TEXT("Failed to spawn %s"), *SpawnClass->GetName());
		}
	}

	if (SpawnedCount > 0)
	{
		LastSpawnTime = World->GetTimeSeconds();
		PlaySpawnSfx();
	}

	return SpawnedCount;
}

template<typename InAllocatorType>
bool AEnemySpawner::IsSpawnPointObstructed(UClass* SpawnClass, const FTransform& SpawnTransform, const TArray<AActor*, InAllocatorType>& SpawnedThisCycle) const
{
	check(SpawnClass);

	auto World = GetWorld();
	check(World);

	// Get bounds of the CDO for the spawn class
	AActor* SpawnCDO = Cast<AActor>(SpawnClass->GetDefaultObject());

	if (!SpawnCDO)
	{
		UE_VLOG_UELOG(this, LogTRAI, Error, TEXT("%s: IsSpawnPointObstructed - SpawnClass=%s was not an actor class!"), *GetName(), *LoggingUtils::GetName(SpawnClass));
		return false;
	}

	FVector BoundsExtent;
	FVector* CachedBoundsExtent = BoundsCache.Find(SpawnCDO);
	if (CachedBoundsExtent)
	{
		BoundsExtent = *CachedBoundsExtent;
	}
	else
	{
		// Compute for first time
		const auto& Bounds = TR::CollisionUtils::GetAABB(*SpawnCDO);
		BoundsExtent = Bounds.GetExtent();

		BoundsExtent.X *= SpawnSafetyBoundsXYMultiplier;
		BoundsExtent.Y *= SpawnSafetyBoundsXYMultiplier;

		BoundsCache.Add(SpawnCDO, BoundsExtent);
	}

	FCollisionQueryParams QueryParams;
	// Ignore any actors spawned this cycle as we've already accounted for not spawning multiple in same spawn location
	// and don't want the safety bounds to interfere
	for (auto Spawned : SpawnedThisCycle)
	{
		QueryParams.AddIgnoredActor(Spawned);
	}

	// Consider any dynamic movable object except for Destructibe as its bounds encompass all the small pieces and it's state is more transient
	// Don't use ECC_WorldDynamic as this is used as the default for a lot of overlap components
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_PhysicsBody);

	const FCollisionShape CollisionShape = FCollisionShape::MakeBox(BoundsExtent);


#if WITH_EDITOR
	// Write out extra debug information to logs
	TArray<FOverlapResult> Overlaps;
	const bool bAnyOverlaps = World->OverlapMultiByObjectType(Overlaps, SpawnTransform.GetLocation(), SpawnTransform.GetRotation(),
		ObjectParams, CollisionShape, QueryParams);
	if (!bAnyOverlaps)
	{
		UE_VLOG_UELOG(this, LogTRAI, Verbose, TEXT("%s: IsSpawnPointObstructed - Spawn point with SpawnClass=%s;Bounds=%s;Transform=%s did not have any obstructions"),
			*GetName(), *LoggingUtils::GetName(SpawnClass), *BoundsExtent.ToCompactString(), *SpawnTransform.ToHumanReadableString());
		return false;
	}

	UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s: IsSpawnPointObstructed - Spawn point with SpawnClass=%s;Bounds=%s;Transform=%s overlapped with %d existing actors"),
		*GetName(), *LoggingUtils::GetName(SpawnClass), *BoundsExtent.ToCompactString(), *SpawnTransform.ToHumanReadableString(), Overlaps.Num());

	bool bLogIndividualOverlaps = UE_LOG_ACTIVE(LogTRAI, Verbose);

#if ENABLE_VISUAL_LOG
	bLogIndividualOverlaps |= FVisualLogger::IsRecording();
#endif

	if (bLogIndividualOverlaps)
	{
		TArray<AActor*> LoggedActors;
		LoggedActors.Reserve(Overlaps.Num());

		for (int32 AddedCount{}; const auto& OverlapResult : Overlaps)
		{
			const auto OverlappedActor = OverlapResult.GetActor();
			// AddUnique returns the Index of existing element or Added so if it's unique it is added to end and will == previous count
			if (!OverlappedActor || LoggedActors.AddUnique(OverlappedActor) < AddedCount)
			{
				continue;
			}

			++AddedCount;

			UE_VLOG_UELOG(this, LogTRAI, Verbose, TEXT("%s: IsSpawnPointObstructed - Spawn point with SpawnClass=%s;Bounds=%s;Transform=%s overlapped with %s"),
				*GetName(), *LoggingUtils::GetName(SpawnClass), *BoundsExtent.ToCompactString(), *SpawnTransform.ToHumanReadableString(),
				*OverlappedActor->GetName());

			UE_VLOG_LOCATION(this, LogTRAI, Verbose, OverlappedActor->GetActorLocation(), 50.0f, FColor::Orange, TEXT("%s"), *OverlappedActor->GetName());
		}
	}

	return true;

#else // not WITH_EDITOR - Disable enhanced logging
	return World->OverlapAnyTestByObjectType(SpawnTransform.GetLocation(), SpawnTransform.GetRotation(),
		ObjectParams, CollisionShape, QueryParams);
#endif
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

	auto World = GetWorld();
	check(World);

	World->OnWorldBeginPlay.AddUObject(this, &ThisClass::GroundSpawnPoints);
}

void AEnemySpawner::GroundSpawnPoints()
{
	UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s: GroundSpawnPoints - GroundSpawnOffset=%f and Z Offset for trace is {%f,%f}"), *GetName(), GroundSpawnOffset, GroundTraceUpOffset, GroundTraceDownOffset);

	for (auto* SpawnLocation : SpawnLocations)
	{
		check(SpawnLocation);

		GroundSpawnPoint(*SpawnLocation);
	}
}

void AEnemySpawner::GroundSpawnPoint(USpawnLocationComponent& SpawnLocation)
{
	auto World = GetWorld();
	check(World);

	FHitResult HitResult;

	const auto& CurrentLocation = SpawnLocation.GetComponentLocation();

	const FVector TraceStart = CurrentLocation + FVector(0, 0, GroundTraceUpOffset);
	const FVector TraceEnd = CurrentLocation - FVector(0, 0, GroundTraceDownOffset);

	if (World->LineTraceSingleByObjectType(HitResult, TraceStart, TraceEnd, TR::CollisionChannel::GroundObjectType))
	{
		const auto& AdjustedLocation = HitResult.Location + HitResult.Normal * GroundSpawnOffset;

		if (!CurrentLocation.Equals(AdjustedLocation, GroundEpsilon))
		{
			UE_VLOG_UELOG(this, LogTRAI, Verbose, TEXT("%s: GroundSpawnPoint: %s - Adjusted ground location from %s -> %s"),
				*GetName(), *SpawnLocation.GetName(), *CurrentLocation.ToCompactString(), *AdjustedLocation.ToCompactString());

			SpawnLocation.SetWorldLocation(AdjustedLocation);

			UE_VLOG_LOCATION(this, LogTRAI, Verbose, SpawnLocation.GetComponentLocation(), 50.0f, FColor::Blue, TEXT("Spawn Location (Adj)"));
		}
		else
		{
			UE_VLOG_LOCATION(this, LogTRAI, Verbose, SpawnLocation.GetComponentLocation(), 50.0f, FColor::Green, TEXT("Spawn Location"));
		}
	}
	else
	{
		UE_VLOG_LOCATION(this, LogTRAI, Warning, CurrentLocation, 100.0f, FColor::Orange, TEXT("Spawner could not find ground!"));
		UE_VLOG_UELOG(this, LogTRAI, Warning, 
			TEXT("%s: GroundSpawnPoint: %s - Could not find ground (Is landscape/floor set to 'Ground' profile?) - no adjustment made to initial position of %s"),
			*GetName(), *SpawnLocation.GetName(), *CurrentLocation.ToCompactString());
	}
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
	UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s: Initialize - NoSpawnZoneComponent=%s"), *GetName(), *LoggingUtils::GetName(InOverlapComponent));

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

void AEnemySpawner::PlaySpawnSfx() const
{
	if (!SpawnSfx)
	{
		UE_VLOG_UELOG(this, LogTRAI, Warning, TEXT("%s - PlaySpawnSfx - No SpawnSfx set"), *GetName());
		return;
	}

	UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s - PlayPickupSfx - Playing %s"), *GetName(), *SpawnSfx->GetName());

	UGameplayStatics::PlaySound2D(GetWorld(), SpawnSfx);
}

namespace
{
	std::optional<FCameraFOVResult> GetCameraFOVFor(const APawn& Pawn)
	{
		auto CameraComponent = Pawn.FindComponentByClass<UCameraComponent>();
		if (!CameraComponent)
		{
			return {};
		}

		return FCameraFOVResult { CameraComponent->GetComponentLocation(), CameraComponent->FieldOfView };
	}

	bool IsInFOV(const APawn& Pawn, const FVector& SpawnReferenceLocation)
	{
		auto FOVOpt = GetCameraFOVFor(Pawn);
		if (!FOVOpt)
		{
			return false;
		}

		const auto FOVHalfAngleRads = FMath::DegreesToRadians(FOVOpt->FOV * 0.5f);

		const auto& ActorForwardVector = Pawn.GetActorForwardVector();
		const auto& ActorLocation = Pawn.GetActorLocation();

		const auto ToSpawnDirection = (SpawnReferenceLocation - ActorLocation).GetSafeNormal();

		const auto ToSpawnHalfAngleRads = FMath::Acos(ToSpawnDirection | ActorForwardVector);

		if (ToSpawnHalfAngleRads > FOVHalfAngleRads)
		{
			return false;
		}

		// Line test against camera to location to see if player can see the given spawn reference location

		auto World = Pawn.GetWorld();
		check(World);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(&Pawn);
		QueryParams.bIgnoreTouches = true;
		QueryParams.bFindInitialOverlaps = false;
		QueryParams.bIgnoreBlocks = false;
		// Only look for hits with environment art
		QueryParams.MobilityType = EQueryMobilityType::Static;

		// LineTraceTestByChannel returns whether a blocking hit was found, so return true if no blocking hit found
		return !World->LineTraceTestByChannel(FOVOpt->Location, SpawnReferenceLocation, ECollisionChannel::ECC_Visibility, QueryParams);
	}

	FRotator GetSpawnActorRotation(const USceneComponent& SpawnLocationComponent, const AActor* LookAtActor)
	{
		const FRotator& SpawnLocationRotation = SpawnLocationComponent.GetComponentRotation();

		if (!LookAtActor)
		{
			return SpawnLocationRotation;
		}

		const FVector LookAtDirection = LookAtActor->GetActorLocation() - SpawnLocationComponent.GetComponentLocation();
		const FRotator LookAtRotator = LookAtDirection.Rotation();

		// Only use the Yaw from the look-at rotator
		return FRotator(SpawnLocationRotation.Pitch, LookAtRotator.Yaw, SpawnLocationRotation.Roll);
	}
}