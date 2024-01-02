// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/XPSpawnerComponent.h"

#include "Subsystems/TankEventsSubsystem.h"
#include "Pickup/XPToken.h"
#include "Pawn/BaseTankPawn.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "TankRampageLogging.h"

UXPSpawnerComponent::UXPSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UXPSpawnerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!XPTokenClass)
	{
		UE_LOG(LogTankRampage, Error, TEXT("%s: XPTokenClass is not set - no XP tokens will be spawned!"), *GetName());
		return;
	}

	auto World = GetWorld();
	check(World);

	if (auto TankEventSubsystem = World->GetSubsystem<UTankEventsSubsystem>(); ensure(TankEventSubsystem))
	{
		TankEventSubsystem->OnTankDestroyed.AddDynamic(this, &ThisClass::OnTankDestroyed);
	}
}

void UXPSpawnerComponent::OnTankDestroyed(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith)
{
	UE_VLOG_UELOG(this, LogTankRampage, Log, TEXT("%s: OnTankDestroyed - DestroyedTank=%s; DestroyedBy=%s; DestroyedWith=%s"),
		*GetName(), *LoggingUtils::GetName(DestroyedTank), *LoggingUtils::GetName(DestroyedBy), *LoggingUtils::GetName(DestroyedWith));

	// Destroyed tank must not be player controlled and destroyer must be the player and not friendly fire from another AI tank
	if (!DestroyedBy || !DestroyedBy->IsPlayerController() || !DestroyedTank || (DestroyedTank->GetController() && DestroyedTank->GetController()->IsPlayerController()))
	{
		return;
	}

	SpawnToken(DestroyedTank->GetActorLocation(), *DestroyedBy);
}

FVector UXPSpawnerComponent::GetSpawnLocation(const FVector& BaseLocation) const
{
	auto World = GetWorld();
	check(World);

	// Spawn at ground
	FCollisionObjectQueryParams Params;
	Params.ObjectTypesToQuery = FCollisionObjectQueryParams::AllStaticObjects | FCollisionObjectQueryParams::AllDynamicObjects;

	FHitResult HitResult;

	if (World->LineTraceSingleByObjectType(
		HitResult,
		BaseLocation + FVector(0, 0, 100),
		BaseLocation - FVector(0, 0, 1000),
		Params
	))
	{
		UE_VLOG_UELOG(this, LogTankRampage, Log, TEXT("%s: SpawnToken - Collided with %s owned by %s - adjusting location from %s to %s"),
			*GetName(), 
			*LoggingUtils::GetName(HitResult.Component.Get()),
			*LoggingUtils::GetName(HitResult.GetActor()),
			*BaseLocation.ToCompactString(), *HitResult.Location.ToCompactString());

		return HitResult.Location;
	}
	else
	{
		UE_VLOG_UELOG(this, LogTankRampage, Warning, TEXT("%s: SpawnToken - Could not find ground to snap token; Location=%s"),
			*GetName(), *BaseLocation.ToCompactString());

		return BaseLocation;
	}
}

void UXPSpawnerComponent::SpawnToken(const FVector& Location, const AController& Owner) const
{
	UE_VLOG_UELOG(this, LogTankRampage, Log, TEXT("%s: SpawnToken - Location=%s; Owner=%s"),
		*GetName(), *Location.ToCompactString(), *Owner.GetName());

	auto World = GetWorld();
	check(World);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Instigator = Owner.GetPawn();

	const auto SpawnLocation = GetSpawnLocation(Location);

	auto Token = World->SpawnActor<AXPToken>(XPTokenClass, SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
	if (Token)
	{
		UE_VLOG_UELOG(this, LogTankRampage, Log, TEXT("%s: SpawnToken - Spawned token=%s with class=%s at Location=%s"),
			*GetName(), *Token->GetName(), *LoggingUtils::GetName(XPTokenClass), *SpawnLocation.ToCompactString());
	}
	else
	{
		UE_VLOG_UELOG(this, LogTankRampage, Warning, TEXT("%s: SpawnToken - Unable to spawn token with class=%s; Location=%s; Owner=%s"),
			*GetName(), *LoggingUtils::GetName(XPTokenClass), *Location.ToCompactString(), *Owner.GetName());
	}
}

