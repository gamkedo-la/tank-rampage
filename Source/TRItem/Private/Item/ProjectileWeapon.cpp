// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/ProjectileWeapon.h"
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

bool UProjectileWeapon::DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName)
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
			[&ActivationSocketName, this, WeakCompReference = MakeWeakObjectPtr(&ActivationReferenceComponent), LoopCount = int32{ 0 }]()mutable
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

void UProjectileWeapon::LaunchProjectile(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName)
{
	const FVector SpawnLocation = ActivationReferenceComponent.GetSocketLocation(ActivationSocketName);
	const FRotator SpawnRotation = ActivationReferenceComponent.GetSocketRotation(ActivationSocketName);

	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	auto World = GetWorld();
	check(World);

	auto ChosenWeaponProjectileClass = ChooseProjectileClass();
	auto SpawnedProjectile = World->SpawnActorDeferred<AProjectile>(ChosenWeaponProjectileClass, SpawnTransform, GetOwner(), GetOwner());

	if (!SpawnedProjectile)
	{
		UE_VLOG_UELOG(GetOuter(), LogTRItem, Warning, TEXT("%s-%s: LaunchProjectile: Unable to spawn projectile %s at %s with rotation=%s"),
			*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(ChosenWeaponProjectileClass), *SpawnLocation.ToCompactString());
		return;
	}

	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s-%s: LaunchProjectile: %s at %s"),
		*LoggingUtils::GetName(GetOuter()), *GetName(),
		*LoggingUtils::GetName(SpawnedProjectile), *SpawnLocation.ToCompactString(), *SpawnRotation.ToCompactString());

	const FProjectileDamageParams ProjectileDamageParams
	{
		.WeaponDamageType = WeaponDamageType,
		.MinDamageAmount = MinDamageAmount,
		.MaxDamageAmount = DamageAmount,
		.DamageInnerRadius = DamageInnerRadius,
		.DamageOuterRadius = DamageOuterRadius,
		.DamageFalloff = DamageFalloff,
		.ImpactImpulseAmountMultiplier = ImpactImpulseAmountMultiplier
	};

	std::optional<FProjectileHomingParams> OptHomingParams;

	if (bIsHoming)
	{
		UpdateHomingTargets(*SpawnedProjectile);

		OptHomingParams = FProjectileHomingParams{
			.MaxSpeedMultiplier = MaxSpeedMultiplier,
			.HomingAcceleration = HomingAcceleration,
			.HomingTargetRefreshInterval = HomingTargetRefreshInterval,
			.Targets = AvailableHomingTargets
		};
	}

	SpawnedProjectile->Initialize(ActivationReferenceComponent, ActivationSocketName, ProjectileDamageParams, OptHomingParams);
	SpawnedProjectile->FinishSpawning(SpawnTransform);

	SpawnedProjectile->Launch(ProjectileLaunchSpeed);
}

void UProjectileWeapon::BeginDestroy()
{
	Super::BeginDestroy();

	ClearProjectileTimer();
}

void UProjectileWeapon::ClearProjectileTimer()
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

void UProjectileWeapon::UpdateHomingTargets(AProjectile& Projectile)
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	Projectile.OnDestroyed.AddDynamic(this, &ThisClass::OnProjectileDestroyed);
	Projectile.OnHomingTargetSelected.AddUObject(this, &ThisClass::OnHomingTargetSelected);

	const float CurrentTimeSeconds = World->GetTimeSeconds();

	if (HomingTargetLastUpdateTime < 0)
	{
		TArray<AActor*> Actors;
		for (const auto& Class : HomingTargetClasses)
		{
			// Actors gets reset inside the function call
			UGameplayStatics::GetAllActorsOfClassWithTag(this, Class, TR::Tags::Alive, Actors);
			AvailableHomingTargets.Append(Actors);

			UE_VLOG_UELOG(GetOuter(), LogTRItem, Verbose, TEXT("%s: UpdateHomingTarget - Init - Found %d actor%s that were alive with class %s"),
				*GetName(), Actors.Num(), LoggingUtils::Pluralize(Actors.Num()), *LoggingUtils::GetName(Class));
		}

		// Don't try to aim toward self
		AvailableHomingTargets.Remove(GetOwner());

		HomingTargetLastUpdateTime = CurrentTimeSeconds;

		UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: UpdateHomingTarget - Init - Found %d total target%s"),
			*GetName(), AvailableHomingTargets.Num(), LoggingUtils::Pluralize(Actors.Num()));
	}
	else if (CurrentTimeSeconds - HomingTargetLastUpdateTime >= HomingTargetsUpdateFrequency)
	{
		for (auto It = AvailableHomingTargets.CreateIterator(); It; ++It)
		{
			auto Actor = *It;
			if (!IsValid(Actor) || Actor->ActorHasTag(TR::Tags::Dead))
			{
				It.RemoveCurrent();
			}
		}

		HomingTargetLastUpdateTime = CurrentTimeSeconds;

		UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: UpdateHomingTarget - Removed dead actors - %d target%s remaining"),
			*GetName(), AvailableHomingTargets.Num(), LoggingUtils::Pluralize(AvailableHomingTargets.Num()));
	}
}

TSubclassOf<AProjectile> UProjectileWeapon::ChooseProjectileClass() const
{
	if (bIsHoming && ensure(WeaponHomingProjectileClass))
	{
		return WeaponHomingProjectileClass;
	}

	return WeaponProjectileClass;
}

void UProjectileWeapon::OnProjectileDestroyed(AActor* Actor)
{
	auto DestroyedProjectile = Cast<AProjectile>(Actor);
	if (!DestroyedProjectile)
	{
		return;
	}

	AActor* DestroyedProjectileHomingTarget{};
	ProjectileTargetMap.RemoveAndCopyValue(DestroyedProjectile, DestroyedProjectileHomingTarget);

	if (!IsValid(DestroyedProjectileHomingTarget))
	{
		return;
	}

	AvailableHomingTargets.Add(DestroyedProjectileHomingTarget);

	for (auto [Projectile, _] : ProjectileTargetMap)
	{
		if (Projectile)
		{
			Projectile->AddAvailableHomingTarget(DestroyedProjectileHomingTarget);
		}
	}
}

void UProjectileWeapon::OnHomingTargetSelected(AProjectile* InProjectile, AActor* InTarget)
{
	check(InProjectile);

	AActor** PreviousHomingTargetFindResult = ProjectileTargetMap.Find(InProjectile);
	AActor* PreviousHomingTarget = PreviousHomingTargetFindResult && IsValid(*PreviousHomingTargetFindResult) ? *PreviousHomingTargetFindResult : nullptr;

	if (InTarget)
	{
		ProjectileTargetMap.Add(InProjectile, InTarget);
		AvailableHomingTargets.Remove(InTarget);
	}
	else
	{
		ProjectileTargetMap.Remove(InProjectile);
	}

	if (PreviousHomingTarget)
	{
		AvailableHomingTargets.Add(PreviousHomingTarget);
	}

	for (auto [Projectile, _] : ProjectileTargetMap)
	{
		if (!Projectile || Projectile == InProjectile)
		{
			continue;
		}

		if (InTarget)
		{
			Projectile->RemoveAvailableHomingTarget(InTarget);
		}
		if (PreviousHomingTarget)
		{
			Projectile->AddAvailableHomingTarget(PreviousHomingTarget);
		}
	}
}
