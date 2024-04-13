// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/LootDropComponent.h"

#include "XPSubsystem.h"
#include "Subsystems/TankEventsSubsystem.h"
#include "Item/ItemSubsystem.h"
#include "Pawn/BaseTankPawn.h"
#include "Pickup/BasePickup.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "TankRampageLogging.h"

#include "Utils/CollisionUtils.h"
#include "Utils/ObjectUtils.h"

#include "Engine/CurveTable.h"
#include "Curves/RealCurve.h"

#include "Kismet/GameplayStatics.h"

#include <cstdio>
#include <optional>

namespace
{
	FRealCurve* FindCurveForLevel(UCurveTable* CurveTable, int32 Level);

	bool ShouldIgnoreOverlap(const FOverlapResult& Overlap);

	const UObject* OverlapKey(const FOverlapResult& Overlap);

	FBox GetObjectAABB(const UObject* Object);
}

struct FSpawnData
{
	FVector BoundsExtent;
	TArray<FVector, TInlineAllocator<2>> SpawnLocations;

	static constexpr int32 IndexInitial = 0;
	static constexpr int32 IndexFinal = 1;
};

struct ULootDropComponent::FSpawnContext
{
	TSubclassOf<ABasePickup> PickupClass{};
	TOptional<FBox> PickupBounds{};
	TOptional<FVector> BiasLocation{};
	FBox OverlapBounds{ EForceInit::ForceInitToZero };
	TArray<FSpawnData>* OtherSpawned{};

	FSpawnContext(const TSubclassOf<ABasePickup>& PickupClass, TArray<FSpawnData>& OtherSpawned) : PickupClass(PickupClass), OtherSpawned(&OtherSpawned) {}
};

// Sets default values for this component's properties
ULootDropComponent::ULootDropComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void ULootDropComponent::BeginPlay()
{
	Super::BeginPlay();


	if (!ValidateConfig())
	{
		return;
	}

	auto World = GetWorld();
	check(World);

	// initialize the first level of drop probabilities
	InitializeLevelData(0);

	if (auto TankEventSubsystem = World->GetSubsystem<UTankEventsSubsystem>(); ensure(TankEventSubsystem))
	{
		TankEventSubsystem->OnTankDestroyed.AddDynamic(this, &ThisClass::OnTankDestroyed);
	}

	if (auto XPSubsystem = World->GetSubsystem<UXPSubsystem>(); ensure(XPSubsystem))
	{
		XPSubsystem->OnXPLevelUp.AddDynamic(this, &ThisClass::OnXPLevelUp);
	}
}

void ULootDropComponent::OnTankDestroyed(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith)
{
	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: OnTankDestroyed - DestroyedTank=%s; DestroyedBy=%s; DestroyedWith=%s"),
		*GetName(), *LoggingUtils::GetName(DestroyedTank), *LoggingUtils::GetName(DestroyedBy), *LoggingUtils::GetName(DestroyedWith));

	// Make sure destroyed tank is not the player - allow drops from AI that kill each other with friendly fire
	if (!DestroyedTank || !DestroyedTank->GetController() || DestroyedTank->GetController()->IsPlayerController())
	{
		UE_VLOG_UELOG(this, LogTankRampage, Log, TEXT("%s: OnTankDestroyed - Skipping player-controlled destroyed tank"));

		return;
	}

	++EnemiesDestroyedThisLevel;
	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: OnTankDestroyed - %d enemies destroyed in level %d"),
		*GetName(), EnemiesDestroyedThisLevel, CurrentLevel + 1);

	auto PlayerController = DestroyedBy && DestroyedBy->IsPlayerController() ? DestroyedBy : UGameplayStatics::GetPlayerController(this, 0);

	TOptional<FVector> SpawnReferenceLocation;

	const auto BaseLocation = GetSpawnBaseLocation(*DestroyedTank, PlayerController, SpawnReferenceLocation);

	SpawnLoot(PlayerController, BaseLocation, SpawnReferenceLocation);
}

void ULootDropComponent::OnXPLevelUp(int32 NewLevel)
{
	UE_VLOG_UELOG(this, LogTankRampage, Log, TEXT("%s: OnXPLevelUp - NewLevel=%d"), *GetName(), NewLevel);

	CurrentLevel = NewLevel;
	EnemiesDestroyedThisLevel = 0;

	InitializeLevelData(NewLevel);
}

void ULootDropComponent::SpawnLoot(const AController* Owner, const FVector& BaseSpawnLocation, const TOptional<FVector>& SpawnReferenceLocation)
{
	auto World = GetWorld();
	check(World);

	TArray<FSpawnData> SpawnedLootBounds;

	for (const auto& Config : LootConfigs)
	{
		const auto& Class = Config.Class;
		const auto DropCurveTable = Config.DropCurveTable;

		// Should have already been removed if invalid
		check(Class && DropCurveTable);

		if (!ShouldSpawnLootClass(Config))
		{
			continue;
		}

		FSpawnContext Context(Class, SpawnedLootBounds);
		Context.BiasLocation = SpawnReferenceLocation;

		FVector InitialSpawnLocation;

		const auto& SpawnLocation = GetSpawnLocation(Context, BaseSpawnLocation, InitialSpawnLocation);

		if (auto Spawned = SpawnLoot(Owner, SpawnLocation, Class); Spawned)
		{
			auto* LevelData = LevelDataByClass.Find(Class);
			check(LevelData);

			++LevelData->Awarded;

			if (!Context.PickupBounds)
			{
				Context.PickupBounds = GetPickupBounds(*Spawned);
			}

			if (Context.PickupBounds)
			{
				// Store the initial spawn selection locations for spawn overlap detection
				SpawnedLootBounds.Add({
					Context.PickupBounds->GetExtent(),
					{ InitialSpawnLocation, Spawned->GetActorLocation() }
				});
			}

			if (auto ItemSubsystem = World->GetSubsystem<UItemSubsystem>(); ensure(ItemSubsystem))
			{
				ItemSubsystem->OnLootSpawned.Broadcast(Spawned);
			}
		}
	}
}

const ABasePickup* ULootDropComponent::SpawnLoot(const AController* Owner, const FVector& SpawnLocation, UClass* PickupClass) const
{
	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: SpawnLoot - PickupClass=%s; SpawnLocation=%s; Owner=%s"),
		*GetName(), *LoggingUtils::GetName(PickupClass), *SpawnLocation.ToCompactString(), *LoggingUtils::GetName(Owner));

	check(PickupClass);

	auto World = GetWorld();
	check(World);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (Owner)
	{
		SpawnParameters.Instigator = Owner->GetPawn();
	}

	auto Pickup = World->SpawnActor<ABasePickup>(PickupClass, SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
	if (Pickup)
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: SpawnLoot - Spawned pickup=%s with class=%s at SpawnLocation=%s"),
			*GetName(), *Pickup->GetName(), *LoggingUtils::GetName(PickupClass), *SpawnLocation.ToCompactString());
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Warning, TEXT("%s: SpawnLoot - Unable to spawn pickup with class=%s; SpawnLocation=%s; Owner=%s"),
			*GetName(), *LoggingUtils::GetName(PickupClass), *SpawnLocation.ToCompactString(), *LoggingUtils::GetName(Owner));
	}

	return Pickup;
}

FVector ULootDropComponent::GetSpawnLocation(FSpawnContext& SpawnContext, const FVector& BaseLocation, FVector& InitialSpawnLocation) const
{
	SpawnContext.PickupBounds = GetPickupBounds(SpawnContext.PickupClass);

	FVector SpawnLocation = InitialSpawnLocation = GetOffsetSpawnLocation(SpawnContext, BaseLocation, FSpawnData::IndexInitial);

	for (int32 i = 0; i < CollisionTestMaxIterations; ++i)
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: GetSpawnLocation - %s - Testing for collision iteration %d / %d with SpawnLocation=%s"),
			*GetName(), *LoggingUtils::GetName(SpawnContext.PickupClass), i + 1, CollisionTestMaxIterations, *SpawnLocation.ToCompactString());

		if (GetCollisionFreeSpawnLocation(SpawnContext, SpawnLocation, i))
		{
			// No further collision
			break;
		}
		else if (i == CollisionTestMaxIterations - 1)
		{
			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Warning, TEXT("%s: GetSpawnLocation - %s - Reached max iterations of %d when testing for collisions - SpawnLocation=%s"),
				*GetName(), *LoggingUtils::GetName(SpawnContext.PickupClass), CollisionTestMaxIterations, *SpawnLocation.ToCompactString());
		}
	}

	return GroundSpawnLocation(SpawnContext.PickupClass, SpawnLocation);
}

FVector ULootDropComponent::GetSpawnBaseLocation(const ABaseTankPawn& DestroyedTank, const AController* DestroyedBy, TOptional<FVector>& OutSpawnReferenceLocation) const
{
	OutSpawnReferenceLocation = GetReferenceActorLocation(DestroyedTank, DestroyedBy);
	const FVector& BaseLocation = DestroyedTank.GetActorLocation();

	if (!OutSpawnReferenceLocation)
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: GetSpawnBaseLocation - No reference location - returning base location of destroyed actor %s -> %s"),
			*GetName(), *DestroyedTank.GetName(), *BaseLocation.ToCompactString());

		return BaseLocation;
	}

	// Offset so that spawn circle bounds moves toward player and tangent to BaseLocation
	const FVector ToPlayer = (*OutSpawnReferenceLocation - BaseLocation).GetSafeNormal2D();

	return BaseLocation + ToPlayer * SpawnRadius;
}

TOptional<FVector> ULootDropComponent::GetReferenceActorLocation(const ABaseTankPawn& DestroyedTank, const AController* DestroyedBy) const
{
	const FVector& BaseLocation = DestroyedTank.GetActorLocation();
	const AActor* ReferenceActor = DestroyedBy ? DestroyedBy->GetPawn() : nullptr;

	if (!ReferenceActor)
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: GetReferenceActorLocation - ReferenceActor is NULL for DestroyedBy=%s; DestroyedTank=%s"),
			*GetName(), *LoggingUtils::GetName(DestroyedBy), *DestroyedTank.GetName());

		return {};
	}

	return ReferenceActor->GetActorLocation();
}

FVector ULootDropComponent::GetLocationWithOffset(const FVector& Location) const
{
	const FVector2D XYOffset = FMath::RandPointInCircle(SpawnRadius);
	return Location + FVector{ XYOffset.X, XYOffset.Y, 0 };
}

FVector ULootDropComponent::GetOffsetSpawnLocation(const FSpawnContext& SpawnContext, const FVector& BaseLocation, int32 LocationOffset) const
{
	int32 i = 0;
	FVector InitialSpawnLocation;

	do
	{
		++i;

		InitialSpawnLocation = GetLocationWithOffset(BaseLocation);

		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: GetOffsetSpawnLocation(%d) - %s - Testing for other loot collision iteration %d / %d with InitialSpawnLocation=%s"),
			*GetName(), LocationOffset, *LoggingUtils::GetName(SpawnContext.PickupClass), i, MultipleLootOverlapCheckMaxIterations, *InitialSpawnLocation.ToCompactString());

		if (!IsOverlappingExistingSpawns(SpawnContext, InitialSpawnLocation, LocationOffset))
		{
			break;
		}
	}
	while (i < MultipleLootOverlapCheckMaxIterations);

	if (i == MultipleLootOverlapCheckMaxIterations)
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Warning, TEXT("%s: GetOffsetSpawnLocation(%d) - %s - Reached max iterations of %d when testing for collisions - InitialSpawnLocation=%s"),
			*GetName(), LocationOffset, *LoggingUtils::GetName(SpawnContext.PickupClass), MultipleLootOverlapCheckMaxIterations, *InitialSpawnLocation.ToCompactString());
	}

	return InitialSpawnLocation;
}

bool ULootDropComponent::IsOverlappingExistingSpawns(const FSpawnContext& SpawnContext, const FVector& Location, int32 LocationIndex) const
{
	check(SpawnContext.OtherSpawned);
	check(LocationIndex >= 0 && LocationIndex <= FSpawnData::IndexFinal);

	if (!SpawnContext.PickupBounds || SpawnContext.OtherSpawned->IsEmpty())
	{
		return false;
	}

	const FBox PickupBox = SpawnContext.PickupBounds->MoveTo(Location);

	for (const auto& ExistingSpawnedData : *SpawnContext.OtherSpawned)
	{
		const FBox ExistingSpawnedBounds = FBox::BuildAABB(ExistingSpawnedData.SpawnLocations[LocationIndex], ExistingSpawnedData.BoundsExtent);

		if (ExistingSpawnedBounds.Intersect(PickupBox))
		{
			return true;
		}
	}

	return false;
}

bool ULootDropComponent::GetCollisionFreeSpawnLocation(FSpawnContext& SpawnContext, FVector& SpawnLocation, int32 Iteration) const
{
	if (!SpawnContext.PickupBounds)
	{
		return true;
	}

	auto World = GetWorld();
	check(World);

	TArray<FOverlapResult> Overlaps;

	const FVector& PickupBoundsExtent = SpawnContext.PickupBounds->GetExtent();

	const auto PickupCollisionShape = FCollisionShape::MakeBox(PickupBoundsExtent);

	World->OverlapMultiByObjectType(
		Overlaps,
		SpawnLocation,
		FQuat{ EForceInit::ForceInit }, // ZeroRotator
		ECollisionChannel::ECC_WorldStatic,
		PickupCollisionShape);

	TSet<const UObject*, DefaultKeyFuncs<const UObject*>, TInlineSetAllocator<64>> OverlapResults;

	for (const auto& OverlapResult : Overlaps)
	{
		const auto Key = OverlapKey(OverlapResult);
		if (!Key)
		{
			continue;
		}

		if (ShouldIgnoreOverlap(OverlapResult))
		{
			continue;
		}

		bool bAlreadyVisited{};
		OverlapResults.Add(Key, &bAlreadyVisited);

		if (!bAlreadyVisited)
		{
			const FBox& ObjectBounds = GetObjectAABB(Key);

			SpawnContext.OverlapBounds += ObjectBounds;

			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Verbose, TEXT("%s: GetCollisionFreeSpawnLocation - %s Overlapped with %s Bounds=%s; TotalOverlapBounds=%s"),
				*GetName(), *Key->GetName(), *LoggingUtils::GetName(SpawnContext.PickupClass),
				*ObjectBounds.ToString(), *SpawnContext.OverlapBounds.ToString());
		}
	}

#if ENABLE_VISUAL_LOG
	// Don't overdraw
	if (FVisualLogger::IsRecording() && Iteration == 0)
	{
		UE_VLOG_BOX(GetOwner(), LogTankRampage, Verbose, SpawnContext.PickupBounds->MoveTo(SpawnLocation), FColor::Blue, TEXT("I: %s"), *SpawnContext.PickupClass->GetName());
	}
#endif
	// Draw overlap
	if (!OverlapResults.IsEmpty())
	{
		UE_VLOG_BOX(GetOwner(), LogTankRampage, Verbose, SpawnContext.OverlapBounds, FColor::Orange, TEXT("O: %s"), *SpawnContext.PickupClass->GetName());
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Verbose, TEXT("%s: GetCollisionFreeSpawnLocation - No overlaps detected with %s at %s"),
			*GetName(), *LoggingUtils::GetName(SpawnContext.PickupClass), *SpawnLocation.ToCompactString());

		// No adjustment required, no further overlap testing required
		return true;
	}


	FVector OverlapCenter, OverlapExtent;
	SpawnContext.OverlapBounds.GetCenterAndExtents(OverlapCenter, OverlapExtent);

	OverlapCenter.Z = SpawnLocation.Z;

	// Discount the Z difference
	const FBox OverlapBounds2D = SpawnContext.OverlapBounds.MoveTo(OverlapCenter);

	const auto PickupBoundsSize2D = PickupBoundsExtent.Size2D();
	const auto ExtentSize = OverlapExtent.Size2D() + PickupBoundsSize2D;

	const auto PushOutDirection = OrientDirectionTowardPlayer(SpawnContext, SpawnLocation, (SpawnLocation - OverlapCenter).GetSafeNormal());
	const auto CircleEdgeLocation = SpawnLocation + PushOutDirection * ExtentSize;

	// Dial-in the push out distance since we are using a circumscribed circle
	// In order to push out needed to make sure we are outside the box as GetClosestPointTo returns the point if inside the box
	const auto ClampedBoxPoint = OverlapBounds2D.GetClosestPointTo(CircleEdgeLocation);
	// Add in the pickup bounds again and offset distance - make sure don't spawn back in the box when randomize the final location
	const auto CandidateLocation = ClampedBoxPoint + PushOutDirection * (PickupBoundsSize2D + SpawnRadius);
	const auto FinalLocation = GetOffsetSpawnLocation(SpawnContext, CandidateLocation, FSpawnData::IndexFinal);

	UE_VLOG_BOX(GetOwner(), LogTankRampage, Verbose, SpawnContext.PickupBounds->MoveTo(FinalLocation), FColor::Green, TEXT("F(%d): %s"), Iteration, *SpawnContext.PickupClass->GetName());
	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: GetCollisionFreeSpawnLocation - %d overlaps detected with %s at %s -> %s"),
		*GetName(), OverlapResults.Num(), *LoggingUtils::GetName(SpawnContext.PickupClass),
		*SpawnLocation.ToCompactString(), *FinalLocation.ToCompactString());

	SpawnLocation = FinalLocation;

	// Overlaps detected in this iteration
	return false;
}

FVector ULootDropComponent::OrientDirectionTowardPlayer(const FSpawnContext& SpawnContext, const FVector& Position, const FVector& Direction) const
{
	if (!SpawnContext.BiasLocation)
	{
		return Direction;
	}

	const auto ToPlayer = *SpawnContext.BiasLocation - Position;

	// Flip vector if not oriented toward player
	return (ToPlayer | Direction) >= 0 ? Direction : -Direction;
}

TOptional<FBox> ULootDropComponent::GetPickupBounds(const TSubclassOf<ABasePickup>& PickupClass) const
{
	check(PickupClass);

	auto PickupCDO = Cast<ABasePickup>(PickupClass->GetDefaultObject());
	if (!ensureMsgf(PickupCDO, TEXT("PickupClass=%s has no CDO!"), *PickupClass->GetName()))
	{
		return {};
	}

	// Get the primitive component as the collider is going to be a larger overlap volume
	// The mesh doesn't have a bounds
	auto PickupMesh = TR::ObjectUtils::FindDefaultComponentByClass<UMeshComponent>(PickupCDO);

	if (!ensureMsgf(PickupMesh, TEXT("PickupClass=%s has no primitive component; CDO=%s"), *PickupClass->GetName(), *PickupCDO->GetName()))
	{
		return {};
	}

	return TR::CollisionUtils::GetAABB(*PickupMesh);
}

TOptional<FBox> ULootDropComponent::GetPickupBounds(const ABasePickup& Pickup) const
{
	// Get the primitive component as the collider is going to be a larger overlap volume
	// The mesh doesn't have a bounds
	auto PickupMesh = Pickup.FindComponentByClass<UMeshComponent>();

	if (!ensureMsgf(PickupMesh, TEXT("Pickup=%s has no primitive component"), *Pickup.GetName()))
	{
		return {};
	}

	return TR::CollisionUtils::GetAABB(*PickupMesh);
}

FVector ULootDropComponent::GroundSpawnLocation(const TSubclassOf<ABasePickup>& PickupClass, const FVector& Location) const
{
	auto World = GetWorld();
	check(World);

	FHitResult HitResult;

	const auto OffsetZ = [&]()
	{
		auto PickupCDO = Cast<ABasePickup>(PickupClass->GetDefaultObject());

		if (!ensureMsgf(PickupCDO, TEXT("PickupClass=%s has no CDO!"), *PickupClass->GetName()))
		{
			return decltype(PickupCDO->GetSpawnOffsetZ()){};
		}

		return PickupCDO->GetSpawnOffsetZ();
	}();

	if (World->LineTraceSingleByObjectType(
		HitResult,
		Location + FVector(0, 0, 100),
		Location - FVector(0, 0, 1000),
		TR::CollisionChannel::GroundObjectType
	))
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: GroundSpawnLocation - Collided with %s owned by %s - adjusting location from %s to %s"),
			*GetName(),
			*LoggingUtils::GetName(HitResult.Component.Get()),
			*LoggingUtils::GetName(HitResult.GetActor()),
			*Location.ToCompactString(), *HitResult.Location.ToCompactString());

		return HitResult.Location + FVector(0, 0, OffsetZ);
	}

	UE_VLOG_UELOG(this, LogTankRampage, Warning, TEXT("%s: GroundSpawnLocation - Could not find ground to snap to; SpawnLocation=%s"),
		*GetName(), *Location.ToCompactString());

	return Location + FVector(0, 0, OffsetZ);
}

bool ULootDropComponent::ShouldSpawnLootClass(const FLootConfig& LootConfig) const
{
	const auto* LevelData = LevelDataByClass.Find(LootConfig.Class);
	check(LevelData);

	auto Curve = LevelData->Curve;
	check(Curve);

	const auto CurveValue = Curve->Eval(EnemiesDestroyedThisLevel);
	const auto Probability = CurveValue - LevelData->Awarded;

	bool bResult;
	if (Probability <= 0)
	{
		bResult = false;
	}
	else if (Probability < 1)
	{
		bResult = FMath::FRand() <= Probability;
	}
	// Guaranteed drop
	else
	{
		bResult = true;
	}

	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: ShouldSpawnLootClass - %s with probability of %f (CurveValue=%f) resulted in %s"),
		*GetName(), *LoggingUtils::GetName(LootConfig.Class), Probability, CurveValue, LoggingUtils::GetBoolString(bResult));

	return bResult;
}

bool ULootDropComponent::ValidateConfig()
{
	int32 ConfigIndex{};

	for (auto It = LootConfigs.CreateIterator(); It; ++It)
	{
		++ConfigIndex;

		bool bValid = true;

		if (!It->Class)
		{
			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Warning, TEXT("%s: LootConfig index %d has no class set!"), *GetName(), ConfigIndex);
			bValid = false;
		}
		if (!ValidateCurveTable(It->DropCurveTable))
		{
			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Warning, TEXT("%s: LootConfig index %d has missing/invalid DropCurveTable=%s !"),
				*GetName(), ConfigIndex, *LoggingUtils::GetName(It->DropCurveTable));
			bValid = false;
		}

		if (!bValid)
		{
			It.RemoveCurrent();
		}
	}

	if (LootConfigs.IsEmpty())
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Error, TEXT("%s: LootClasses is not set or all are invalid - no loot items will be spawned!"), *GetName());
		return false;
	}

	return true;
}

bool ULootDropComponent::ValidateCurveTable(UCurveTable* CurveTable) const
{
	if (!CurveTable)
	{
		return false;
	}

	// Need to have at least one entry at Level index 0
	if (!FindCurveForLevel(CurveTable, 0))
	{
		return false;
	}

	return true;
}

void ULootDropComponent::InitializeLevelData(int32 Level)
{
	for (const auto& LootConfig : LootConfigs)
	{
		auto& LevelData = LevelDataByClass.FindOrAdd(LootConfig.Class);
		LevelData.Awarded = 0;

		auto NewLevelCurve = FindCurveForLevel(LootConfig.DropCurveTable, Level);

		// Use new curve if available; otherwise, carry over from previous
		if (NewLevelCurve)
		{
			LevelData.Curve = NewLevelCurve;

			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: InitializeLevelData: Level=%d; Class=%s; Using new curve defined for level from %s"),
				*GetName(), Level, *LoggingUtils::GetName(LootConfig.Class), *LoggingUtils::GetName(LootConfig.DropCurveTable));
		}
		else if (LevelData.Curve)
		{
			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: InitializeLevelData: Level=%d; Class=%s; Carrying over previous level curve from %s"),
				*GetName(), Level, *LoggingUtils::GetName(LootConfig.Class), *LoggingUtils::GetName(LootConfig.DropCurveTable));
		}
		else
		{
			checkf(false, TEXT("Level 0 should have been asserted on CurveTable=%s"), *LoggingUtils::GetName(LootConfig.DropCurveTable));
		}
	}
}

namespace
{
	FRealCurve* FindCurveForLevel(UCurveTable* CurveTable, int32 Level)
	{
		check(CurveTable);

		char buf[16];
		std::snprintf(buf, sizeof(buf), "%d", Level);

		const FString Key = FString::Printf(TEXT("%d"), Level);

#if WITH_EDITOR
		FString Context = FString::Printf(TEXT("FindCurveForLevel: %s->%d"), *LoggingUtils::GetName(CurveTable), Level);
		return CurveTable->FindCurve(FName(buf), Context, Level == 0);
#else
		return CurveTable->FindCurveUnchecked(FName(buf));
#endif
	}

	bool ShouldIgnoreOverlap(const FOverlapResult& Overlap)
	{
		auto Component = Overlap.GetComponent();
		if (!Component)
		{
			return false;
		}

		// Ignore the ground for checking for overlaps 
		// and only consider blocks with the player tank
		return Component->GetCollisionObjectType() == TR::CollisionChannel::GroundObjectType ||
			Component->GetCollisionResponseToChannel(ECC_Pawn) != ECollisionResponse::ECR_Block;
	}

	inline const UObject* OverlapKey(const FOverlapResult& Overlap)
	{
		const UObject* Key = Overlap.GetActor();
		return Key ? Key : Overlap.GetComponent();
	}

	FBox GetObjectAABB(const UObject* Object)
	{
		const auto ActorResult = Cast<AActor>(Object);
		if (ActorResult)
		{
			return TR::CollisionUtils::GetAABB(*ActorResult);
		}

		const auto ComponentResult = Cast<USceneComponent>(Object);
		if (ComponentResult)
		{
			return TR::CollisionUtils::GetAABB(*ComponentResult);
		}

		// Invalid
		return FBox{ EForceInit::ForceInitToZero };
	}
}