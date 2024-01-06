// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Weapon.h"
#include "Item/ItemDataAsset.h"
#include "TRTags.h"

#include "Projectile.h"
#include "Kismet/GameplayStatics.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

#include <optional>

namespace
{
	constexpr float HomingTargetsUpdateFrequency = 3.0f;
}

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

	auto ChosenWeaponProjectileClass = ChooseProjectileClass();
	auto SpawnedProjectile = World->SpawnActor<AProjectile>(ChosenWeaponProjectileClass, SpawnLocation, SpawnRotation, SpawnParameters);

	if (!SpawnedProjectile)
	{
		UE_VLOG_UELOG(GetOuter(), LogTRItem, Warning, TEXT("%s-%s: LaunchProjectile: Unable to spawn projectile %s at %s with rotation=%s"),
			*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(ChosenWeaponProjectileClass), *SpawnLocation.ToCompactString());
		return;
	}

	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s-%s: LaunchProjectile: %s at %s"),
		*LoggingUtils::GetName(GetOuter()), *GetName(),
		*LoggingUtils::GetName(ChosenWeaponProjectileClass), *SpawnLocation.ToCompactString(), *SpawnRotation.ToCompactString());

	const FProjectileDamageParams ProjectileDamageParams
	{
		.WeaponDamageType = WeaponDamageType,
		.MinDamageAmount = MinDamageAmount,
		.MaxDamageAmount = DamageAmount,
		.DamageInnerRadius = DamageInnerRadius,
		.DamageOuterRadius = DamageOuterRadius,
		.DamageFalloff = DamageFalloff
	};

	std::optional<FProjectileHomingParams> OptHomingParams;

	if (bIsHoming)
	{
		UpdateHomingTargets();

		OptHomingParams = FProjectileHomingParams{
			.MaxSpeedMultiplier = MaxSpeedMultiplier,
			.HomingAcceleration = HomingAcceleration,
			.HomingTargetRefreshInterval = HomingTargetRefreshInterval,
			.Targets = HomingTargets
		};
	}

	SpawnedProjectile->Initialize(ActivationReferenceComponent, ActivationSocketName, ProjectileDamageParams, OptHomingParams);
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

void UWeapon::UpdateHomingTargets() const
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CurrentTimeSeconds = World->GetTimeSeconds();

	if (HomingTargetLastUpdateTime < 0)
	{
		TArray<AActor*> Actors;
		for (const auto& Class : HomingTargetClasses)
		{
			// Actors gets reset inside the function call
			UGameplayStatics::GetAllActorsOfClassWithTag(this, Class, TR::Tags::Alive, Actors);
			HomingTargets.Append(Actors);

			UE_VLOG_UELOG(GetOuter(), LogTRItem, Verbose, TEXT("%s: UpdateHomingTarget - Init - Found %d actor%s that were alive with class %s"),
				*GetName(), Actors.Num(), LoggingUtils::Pluralize(Actors.Num()), *LoggingUtils::GetName(Class));
		}

		// Don't try to aim toward self
		HomingTargets.Remove(GetOwner());

		HomingTargetLastUpdateTime = CurrentTimeSeconds;

		UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: UpdateHomingTarget - Init - Found %d total target%s"),
			*GetName(), HomingTargets.Num(), LoggingUtils::Pluralize(Actors.Num()));
	}
	else if(CurrentTimeSeconds - HomingTargetLastUpdateTime >= HomingTargetsUpdateFrequency)
	{
		HomingTargets.RemoveAll([](auto Actor)
		{
			return !IsValid(Actor) || Actor->ActorHasTag(TR::Tags::Dead);
		});

		HomingTargetLastUpdateTime = CurrentTimeSeconds;

		UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: UpdateHomingTarget - Removed dead actors - %d target%s remaining"),
			*GetName(), HomingTargets.Num(), LoggingUtils::Pluralize(HomingTargets.Num()));
	}
}

TSubclassOf<AProjectile> UWeapon::ChooseProjectileClass() const
{
	if (bIsHoming && ensure(WeaponHomingProjectileClass))
	{
		return WeaponHomingProjectileClass;
	}

	return WeaponProjectileClass;
}
