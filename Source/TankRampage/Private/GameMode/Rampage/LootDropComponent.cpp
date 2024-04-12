// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/LootDropComponent.h"

#include "XPSubsystem.h"
#include "Subsystems/TankEventsSubsystem.h"
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

namespace
{
	FRealCurve* FindCurveForLevel(UCurveTable* CurveTable, int32 Level);

	bool ShouldIgnoreOverlap(const FOverlapResult& Overlap);
}

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

	SpawnLoot(PlayerController, DestroyedTank->GetActorLocation());
}

void ULootDropComponent::OnXPLevelUp(int32 NewLevel)
{
	UE_VLOG_UELOG(this, LogTankRampage, Log, TEXT("%s: OnXPLevelUp - NewLevel=%d"), *GetName(), NewLevel);

	CurrentLevel = NewLevel;
	EnemiesDestroyedThisLevel = 0;

	InitializeLevelData(NewLevel);
}

void ULootDropComponent::SpawnLoot(const AController* Owner, const FVector& BaseSpawnLocation)
{
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

		const auto& SpawnLocation = GetSpawnLocation(Class, BaseSpawnLocation);

		if (auto Spawned = SpawnLoot(Owner, SpawnLocation, Class); Spawned)
		{
			auto* LevelData = LevelDataByClass.Find(Class);
			check(LevelData);

			++LevelData->Awarded;
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

FVector ULootDropComponent::GetSpawnLocation(const TSubclassOf<ABasePickup>& PickupClass, const FVector& BaseLocation) const
{
	const FVector OffsetSpawnLocation = GetSpawnOffsetLocation(PickupClass, BaseLocation);

	return GroundSpawnLocation(PickupClass, OffsetSpawnLocation);
}

FVector ULootDropComponent::GetSpawnOffsetLocation(const TSubclassOf<ABasePickup>& PickupClass, const FVector& BaseLocation) const
{
	const FVector InitialOffsetLocation = { FMath::FRandRange(-SpawnRadius, SpawnRadius), FMath::FRandRange(-SpawnRadius, SpawnRadius), 0.0f };
	const FVector InitialSpawnLocation = BaseLocation + InitialOffsetLocation;

	const auto& PickupBoundsOptional = GetPickupBounds(PickupClass);
	if (!PickupBoundsOptional)
	{
		return InitialSpawnLocation;
	}

	auto World = GetWorld();
	check(World);

	TArray<FOverlapResult> Overlaps;

	const FVector& PickupBoundsExtent = PickupBoundsOptional->GetExtent();

	const auto PickupCollisionShape = FCollisionShape::MakeBox(PickupBoundsExtent);

	World->OverlapMultiByObjectType(
		Overlaps,
		InitialSpawnLocation,
		FQuat{ EForceInit::ForceInit }, // ZeroRotator
		ECollisionChannel::ECC_WorldStatic,
		PickupCollisionShape);

	FBox OverlapBounds{ EForceInit::ForceInitToZero };
	TSet<AActor*, DefaultKeyFuncs<AActor*>, TInlineSetAllocator<64>> OverlappedActors;

	for (const auto& OverlapResult : Overlaps)
	{
		auto Actor = OverlapResult.GetActor();
		if (!Actor)
		{
			continue;
		}

		if (ShouldIgnoreOverlap(OverlapResult))
		{
			continue;
		}

		bool bAlreadyVisited{};
		OverlappedActors.Add(Actor, &bAlreadyVisited);

		if (!bAlreadyVisited)
		{
			const FBox& ActorBounds = TR::CollisionUtils::GetAABB(*Actor);
			OverlapBounds += ActorBounds;

			UE_VLOG_UELOG(GetOwner(), LogTankRampage, Verbose, TEXT("%s: GetSpawnOffsetLocation - %s Overlapped with %s Bounds=%s; TotalOverlapBounds=%s"),
				*GetName(), *Actor->GetName(), *LoggingUtils::GetName(PickupClass),
				*ActorBounds.ToString(), *OverlapBounds.ToString());
		}
	}

	UE_VLOG_BOX(GetOwner(), LogTankRampage, Verbose, PickupBoundsOptional->MoveTo(InitialSpawnLocation), FColor::Blue, TEXT("I: %s"), *PickupClass->GetName());

	// Draw overlap
	if (!OverlappedActors.IsEmpty())
	{
		UE_VLOG_BOX(GetOwner(), LogTankRampage, Verbose, OverlapBounds, FColor::Orange, TEXT("O: %s"), *PickupClass->GetName());
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Verbose, TEXT("%s: GetSpawnOffsetLocation - No overlaps detected with %s at %s"),
			*GetName(), *LoggingUtils::GetName(PickupClass), *InitialSpawnLocation.ToCompactString());

		return InitialSpawnLocation;
	}

	FVector OverlapCenter, OverlapExtent;
	OverlapBounds.GetCenterAndExtents(OverlapCenter, OverlapExtent);

	// Discount the Z
	OverlapCenter.Z = InitialSpawnLocation.Z;
	const auto ExtentSize = OverlapExtent.Size2D() + PickupBoundsExtent.Size2D();

	const auto FromOverlapped = (InitialSpawnLocation - OverlapCenter);
	const auto FromOverlappedDirection = FromOverlapped.GetSafeNormal();
	const auto CircleEdgeLocation = FromOverlappedDirection * ExtentSize;

	const auto PushOutOffset = CircleEdgeLocation - FromOverlapped;
	const auto FinalLocation = InitialSpawnLocation + PushOutOffset;

	UE_VLOG_BOX(GetOwner(), LogTankRampage, Verbose, PickupBoundsOptional->MoveTo(FinalLocation), FColor::Green, TEXT("F: %s"), *PickupClass->GetName());
	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: GetSpawnOffsetLocation - %d overlaps detected with %s at %s -> %s"),
		*GetName(), OverlappedActors.Num(), *LoggingUtils::GetName(PickupClass),
		*InitialSpawnLocation.ToCompactString(), *FinalLocation.ToCompactString());

	return FinalLocation;
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

FVector ULootDropComponent::GroundSpawnLocation(const TSubclassOf<ABasePickup>& PickupClass, const FVector& Location) const
{
	auto World = GetWorld();
	check(World);

	FHitResult HitResult;

	FCollisionResponseParams ResponseParams;
	auto& CollisionResponse = ResponseParams.CollisionResponse;

	CollisionResponse.SetAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionResponse.WorldStatic = ECollisionResponse::ECR_Block;

	const auto OffsetZ = [&]()
	{
		auto PickupCDO = Cast<ABasePickup>(PickupClass->GetDefaultObject());

		if (!ensureMsgf(PickupCDO, TEXT("PickupClass=%s has no CDO!"), *PickupClass->GetName()))
		{
			return decltype(PickupCDO->GetSpawnOffsetZ()){};
		}

		return PickupCDO->GetSpawnOffsetZ();
	}();

	if (World->LineTraceSingleByChannel(
		HitResult,
		Location + FVector(0, 0, 100),
		Location - FVector(0, 0, 1000),
		ECollisionChannel::ECC_Visibility,
		{},
		ResponseParams
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

		// Only consider blocks with the player tank
		if (Component->GetCollisionResponseToChannel(ECC_Pawn) != ECollisionResponse::ECR_Block)
		{
			return true;
		}

		auto Actor = Overlap.GetActor();
		if (!Actor)
		{
			return false;
		}

		// Ignore specific actors
		const auto& ActorName = Actor->GetName();

		return ActorName.Contains("Landscape")
#if WITH_EDITOR
			|| ActorName.Contains("Floor")
#endif
		;
	}
}