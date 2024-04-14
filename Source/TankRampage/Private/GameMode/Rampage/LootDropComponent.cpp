// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/LootDropComponent.h"

#include "XPSubsystem.h"
#include "Subsystems/TankEventsSubsystem.h"
#include "Pawn/BaseTankPawn.h"
#include "Pickup/BasePickup.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "TankRampageLogging.h"

#include "Engine/CurveTable.h"
#include "Curves/RealCurve.h"

#include <cstdio>

namespace
{
	FRealCurve* FindCurveForLevel(UCurveTable* CurveTable, int32 Level);
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

	// TODO: DestroyedBy could be NULL or be an AIController
	SpawnLoot(DestroyedBy, DestroyedTank->GetActorLocation());
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

		const auto& SpawnLocation = GetSpawnLocation(BaseSpawnLocation);

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

	// TODO: Determine if it matters if this is NULL
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

FVector ULootDropComponent::GetSpawnLocation(const FVector& BaseLocation) const
{
	auto World = GetWorld();
	check(World);

	// Spawn at ground
	FCollisionObjectQueryParams Params;
	Params.ObjectTypesToQuery = FCollisionObjectQueryParams::AllStaticObjects | FCollisionObjectQueryParams::AllDynamicObjects;

	const FVector Offset = { FMath::FRandRange(-SpawnRadius, SpawnRadius), FMath::FRandRange(-SpawnRadius, SpawnRadius), 0.0f };
	const FVector& RandomizedLocation = BaseLocation + Offset;

	FHitResult HitResult;

	if (World->LineTraceSingleByObjectType(
		HitResult,
		RandomizedLocation + FVector(0, 0, 100),
		RandomizedLocation - FVector(0, 0, 1000),
		Params
	))
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s: GetSpawnLocation - Collided with %s owned by %s - adjusting location from %s to %s"),
			*GetName(),
			*LoggingUtils::GetName(HitResult.Component.Get()),
			*LoggingUtils::GetName(HitResult.GetActor()),
			*RandomizedLocation.ToCompactString(), *HitResult.Location.ToCompactString());

		return HitResult.Location;
	}
	else
	{
		UE_VLOG_UELOG(this, LogTankRampage, Warning, TEXT("%s: GetSpawnLocation - Could not find ground to snap token; SpawnLocation=%s"),
			*GetName(), *RandomizedLocation.ToCompactString());
	}

	return RandomizedLocation;
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
}