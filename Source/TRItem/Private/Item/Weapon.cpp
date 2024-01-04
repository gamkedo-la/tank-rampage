// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Weapon.h"
#include "Item/ItemDataAsset.h"

#include "Projectile.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

bool UWeapon::DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName)
{
	if (!ensure(WeaponProjectileClass))
	{
		return false;
	}

	if (ProjectileCount == 1)
	{
		LaunchProjectile(ActivationReferenceComponent, ActivationSocketName);
	}
	else
	{
		auto World = GetWorld();
		check(World);

		bIsFiring = true;

		World->GetTimerManager().SetTimer(LaunchDelayTimerHandle, FTimerDelegate::CreateWeakLambda(this,
			[&ActivationSocketName, this, WeakCompReference = MakeWeakObjectPtr(&ActivationReferenceComponent), LoopCount = int32{0}]()mutable
			{
				// Have extra delay period between next possible activation to avoid possibility of cooldown being <= fire delay * projectileCount
				if (!WeakCompReference.IsValid() || LoopCount >= ProjectileCount + 1)
				{
					ClearProjectileTimer();
				}
				else if (LoopCount < ProjectileCount)
				{
					LaunchProjectile(*WeakCompReference, ActivationSocketName);
				}
				++LoopCount;

			}), ProjectileLaunchPeriod, true);
	}

	return true;
}

void UWeapon::LaunchProjectile(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName) const
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
		UE_VLOG_UELOG(GetOuter(), LogTRItem, Warning, TEXT("%s-%s: LaunchProjectile: Unable to spawn projectile %s at %s with rotation=%s"),
			*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(WeaponProjectileClass), *SpawnLocation.ToCompactString());
		return;
	}

	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s-%s: LaunchProjectile: %s at %s"),
		*LoggingUtils::GetName(GetOuter()), *GetName(),
		*LoggingUtils::GetName(WeaponProjectileClass), *SpawnLocation.ToCompactString(), *SpawnRotation.ToCompactString());

	const FProjectileDamageParams ProjectileDamageParams
	{
		.WeaponDamageType = WeaponDamageType,
		.MinDamageAmount = MinDamageAmount,
		.MaxDamageAmount = DamageAmount,
		.DamageInnerRadius = DamageInnerRadius,
		.DamageOuterRadius = DamageOuterRadius,
		.DamageFalloff = DamageFalloff
	};

	SpawnedProjectile->Initialize(ActivationReferenceComponent, ActivationSocketName, ProjectileDamageParams);
	SpawnedProjectile->Launch(ProjectileLaunchSpeed);
}

void UWeapon::BeginDestroy()
{
	Super::BeginDestroy();

	ClearProjectileTimer();
}

void UWeapon::ClearProjectileTimer()
{
	bIsFiring = false;

	if (auto World = GetWorld(); World)
	{
		World->GetTimerManager().ClearTimer(LaunchDelayTimerHandle);
	}
	else
	{
		LaunchDelayTimerHandle.Invalidate();
	}
}
