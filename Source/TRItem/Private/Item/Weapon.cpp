// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Weapon.h"
#include "Item/ItemDataAsset.h"

#include "Projectile.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

bool UWeapon::DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName)
{
	const FVector SpawnLocation = ActivationReferenceComponent.GetSocketLocation(ActivationSocketName);
	const FRotator SpawnRotation = ActivationReferenceComponent.GetSocketRotation(ActivationSocketName);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Instigator = GetOwner();
	SpawnParameters.Owner = GetOwner();

	auto World = GetWorld();
	check(World);

	auto SpawnedProjectile = World->SpawnActor<AProjectile>(WeaponProjectileClass, SpawnLocation, SpawnRotation, SpawnParameters);

	if (!SpawnedProjectile)
	{
		UE_VLOG_UELOG(this, LogTRItem, Warning, TEXT("%s: DoActivation: Unable to spawn projectile %s at %s with rotation=%s"),
			*GetName(), *LoggingUtils::GetName(WeaponProjectileClass), *SpawnLocation.ToCompactString());
		return false;
	}

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: DoActivation: %s at %s"), *GetName(),
		*LoggingUtils::GetName(WeaponProjectileClass), *SpawnLocation.ToCompactString(), *SpawnRotation.ToCompactString());

	SpawnedProjectile->Initialize(ActivationReferenceComponent, ActivationSocketName);
	SpawnedProjectile->Launch(ProjectileLaunchSpeed);

	return true;
}
