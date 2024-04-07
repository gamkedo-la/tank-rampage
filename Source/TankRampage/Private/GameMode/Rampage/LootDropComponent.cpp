// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/LootDropComponent.h"

#include "XPSubsystem.h"
#include "Subsystems/TankEventsSubsystem.h"
#include "Pawn/BaseTankPawn.h"
#include "Pickup/BasePickup.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "TankRampageLogging.h"

// Sets default values for this component's properties
ULootDropComponent::ULootDropComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void ULootDropComponent::BeginPlay()
{
	Super::BeginPlay();

	// make sure all classes set
	{
		int32 LootClassIndex{};

		for (auto It = LootClasses.CreateIterator(); It; ++It)
		{
			++LootClassIndex;
			const auto& Class = *It;

			if (!Class)
			{
				UE_VLOG_UELOG(GetOwner(), LogTankRampage, Warning, TEXT("%s: LootClasses index %d is empty!"), *GetName(), LootClassIndex);
				It.RemoveCurrent();
			}
		}
	}

	if (LootClasses.IsEmpty())
	{
		UE_VLOG_UELOG(GetOwner(), LogTankRampage, Error, TEXT("%s: LootClasses is not set or all are invalid - no loot items will be spawned!"), *GetName());
		return;
	}

	auto World = GetWorld();
	check(World);

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
}

void ULootDropComponent::SpawnLoot(const AController* Owner, const FVector& BaseSpawnLocation) const
{
	for (const auto& Class : LootClasses)
	{
		// Should have already been removed if invalid
		check(Class);

		if (!ShouldSpawnLootClass(Class))
		{
			continue;
		}

		const auto& SpawnLocation = GetSpawnLocation(BaseSpawnLocation);

		SpawnLoot(Owner, SpawnLocation, Class);
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

bool ULootDropComponent::ShouldSpawnLootClass(const UClass* PickupClass) const
{
	return FMath::FRand() <= DropProbability;
}
