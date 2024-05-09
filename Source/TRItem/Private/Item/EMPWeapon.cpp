// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/EMPWeapon.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"


#include <limits>

bool UEMPWeapon::DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName)
{
	PlayActivationVfx();
	PlaySfxAtActorLocation(ActivationSfx);

	auto AffectedEnemies = SweepForAffectedEnemies();

	UE_VLOG_UELOG(GetOwner(), LogTRItem, Log, TEXT("%s-%s: DoActivation: Selected %d enemies"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), AffectedEnemies.Num());

	if (AffectedEnemies.IsEmpty())
	{
		return true;
	}

	auto World = GetWorld();
	check(World);

	const float EffectEndGameTime = World->GetTimeSeconds() + EffectDuration;

	const FGameplayTagContainer DebuffTagsContainer = FGameplayTagContainer::CreateFromArray(DebuffTags);

	for (auto Enemy : AffectedEnemies)
	{
		ApplyEffectToEnemy(Enemy, EffectEndGameTime, DebuffTagsContainer);
	}

	if (!TagExpirationHandle.IsValid())
	{
		ScheduleStunRemoval(EffectDuration);

		return true;
	}

	return true;
}

void UEMPWeapon::BeginDestroy()
{
	Super::BeginDestroy();

	if (auto World = GetWorld(); World)
	{
		World->GetTimerManager().ClearTimer(TagExpirationHandle);
	}
	else
	{
		TagExpirationHandle.Invalidate();
	}
}

TArray<APawn*> UEMPWeapon::SweepForAffectedEnemies() const
{
	auto World = GetWorld();
	check(World);
	const auto SweepLocation = GetOwner()->GetActorLocation();

	const FCollisionShape Shape = FCollisionShape::MakeSphere(InfluenceRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	TArray<FOverlapResult> Overlaps;

	World->OverlapMultiByObjectType(Overlaps, SweepLocation,
		FQuat::Identity, ECollisionChannel::ECC_Pawn, Shape, QueryParams);

	TArray<APawn*> SelectedPawns;

	for (const auto& OverlapResult : Overlaps)
	{
		auto Pawn = Cast<APawn>(OverlapResult.GetActor());
		if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
		{
			continue;
		}

		SelectedPawns.AddUnique(Pawn);
	}

	return SelectedPawns;
}

void UEMPWeapon::CheckRemoveStunTag()
{
	UE_VLOG_UELOG(GetOwner(), LogTRItem, Log, TEXT("%s-%s: CheckRemoveStunTag: Active on %d enemies"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), AffectedActors.Num());

	auto World = GetWorld();
	check(World);

	const float CurrentTimeSeconds = World->GetTimeSeconds();

	const FGameplayTagContainer DebuffTagsContainer = FGameplayTagContainer::CreateFromArray(DebuffTags);

	for (auto MapIt = AffectedActors.CreateIterator(); MapIt; ++MapIt)
	{
		auto ASC = MapIt->Key;
		if (!ASC)
		{
			MapIt.RemoveCurrent();
			continue;
		}

		auto& Entry = MapIt->Value;

		// expired
		if (Entry.EndGameTimeSeconds <= CurrentTimeSeconds)
		{
			UE_VLOG_UELOG(GetOwner(), LogTRItem, Log, TEXT("%s-%s: CheckRemoveStunTags: Removing %s from %s"),
				*LoggingUtils::GetName(GetOwner()), *GetName(),
				*DebuffTagsContainer.ToString(), *LoggingUtils::GetName(ASC->GetOwner()));

			ASC->RemoveLooseGameplayTags(DebuffTagsContainer);
			// stop the Vfx
			if (Entry.Vfx)
			{
				Entry.Vfx->Deactivate();
			}

			MapIt.RemoveCurrent();
		}
	}

	// if there are still some not expired, schedule for nearest expiration
	if (AffectedActors.IsEmpty())
	{
		TagExpirationHandle.Invalidate();
		return;
	}

	float MinTime = std::numeric_limits<float>::max();
	for(const auto& [_, Entry] : AffectedActors)
	{
		if (Entry.EndGameTimeSeconds < MinTime)
		{
			MinTime = Entry.EndGameTimeSeconds;
		}
	}

	const auto MinDeltaTime = MinTime - CurrentTimeSeconds;
	checkf(MinDeltaTime > 0, TEXT("MinDeltaTime <= 0: MinTime=%f; CurrentTimeSeconds=%f"),
		MinTime, CurrentTimeSeconds);

	ScheduleStunRemoval(MinDeltaTime);
}

void UEMPWeapon::ScheduleStunRemoval(float DeltaTime)
{
	auto World = GetWorld();
	check(World);

	UE_VLOG_UELOG(GetOwner(), LogTRItem, Log, TEXT("%s-%s: ScheduleStunRemoval: DeltaTime=%fs; AffectedActors=%d"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		DeltaTime, AffectedActors.Num());

	World->GetTimerManager().SetTimer(TagExpirationHandle, this, &ThisClass::CheckRemoveStunTag, DeltaTime, false);
}

void UEMPWeapon::ApplyEffectToEnemy(AActor* Enemy, float EffectEndGameTimeSeconds, const FGameplayTagContainer& DebuffTagsContainer)
{
	check(Enemy);

	auto ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy);
	// already checked in the sweep
	check(ASC);

	// TODO: Do this from a gameplay effect
	ASC->AddLooseGameplayTags(DebuffTagsContainer);

	auto NiagaraComponent = PlayAffectedEnemyVfx(Enemy);

	AffectedActors.Add(ASC, { NiagaraComponent, EffectEndGameTimeSeconds});
}

#pragma region Niagara Vfx

void UEMPWeapon::PlayActivationVfx()
{
	if (!ensureMsgf(ActivationVfx, TEXT("%s: PlayActivationVfx - ActivationVfx is not set"), *GetName()))
	{
		return;
	}

	auto OwningActor = GetOwner();
	check(OwningActor);

	const auto& SpawnLocation = OwningActor->GetActorLocation();

	UE_VLOG_UELOG(OwningActor, LogTRItem, Log, TEXT("%s: PlayActivationVfx: %s playing at %s"), *GetName(), *ActivationVfx->GetName(), *SpawnLocation.ToCompactString());

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ActivationVfx, SpawnLocation, OwningActor->GetActorRotation());

	if (!NiagaraComp)
	{
		UE_VLOG_UELOG(OwningActor, LogTRItem, Log, TEXT("%s: PlayActivationVfx: Could not play %s"), *GetName(), *ActivationVfx->GetName());

		return;
	}

	if (!EffectRadiusName.IsNone())
	{
		UE_VLOG_UELOG(OwningActor, LogTRItem, Verbose, TEXT("%s: PlayActivationVfx: (%s,%s) - Setting effect radius parameter Parameter: %s -> %fm"),
			*GetName(), *ActivationVfx->GetName(), *NiagaraComp->GetName(), *EffectRadiusName.ToString(), InfluenceRadius / 100);

		NiagaraComp->SetFloatParameter(EffectRadiusName, InfluenceRadius);
	}

	if (!OwnerRelativeVelocityName.IsNone())
	{
		const FVector& WorldVelocity = OwningActor->GetVelocity();

		UE_VLOG_UELOG(OwningActor, LogTRItem, Verbose, TEXT("%s: PlayActivationVfx: (%s,%s) - Setting relative velocity parameter Parameter: %s -> %s"),
			*GetName(), *ActivationVfx->GetName(), *NiagaraComp->GetName(),
			*OwnerRelativeVelocityName.ToString(), *WorldVelocity.ToCompactString());

		// This actually needs to be the world velocity and not be relative to the actor's space
		NiagaraComp->SetVectorParameter(OwnerRelativeVelocityName, WorldVelocity);
	}

	UE_VLOG_UELOG(OwningActor, LogTRItem, Log, TEXT("%s: PlayActivationVfx: %s playing NiagaraComponent=%s"),
		*GetName(), *ActivationVfx->GetName(), *NiagaraComp->GetName());
}

UNiagaraComponent* UEMPWeapon::PlayAffectedEnemyVfx(AActor* Enemy)
{
	check(Enemy);

	if (!ensureMsgf(AffectedEnemyVfx, TEXT("%s: PlayAffectedEnemyVfx - AffectedEnemyVfx is not set"), *GetName()))
	{
		return nullptr;
	}

	// Spawn the EMP effect attached to the enemy and don't auto destroy since it is a looping effect that we will deactivate once the effect expires
	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		AffectedEnemyVfx, Enemy->GetRootComponent(), NAME_None,
		FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::Type::SnapToTarget, false);

	if (!NiagaraComp)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRItem, Log, TEXT("%s: PlayAffectedEnemyVfx: Could not play %s for Enemy=%s"), *GetName(), *ActivationVfx->GetName(), *Enemy->GetName());

		return nullptr;
	}

	if (!EnemyRadiusName.IsNone())
	{
		FVector Origin, Extent;
		Enemy->GetActorBounds(true, Origin, Extent);
		const float EnemyRadius = FMath::Max3(Extent.X, Extent.Y, Extent.Z);

		NiagaraComp->SetFloatParameter(EnemyRadiusName, EnemyRadius);

		UE_VLOG_UELOG(GetOwner(), LogTRItem, Verbose, TEXT("%s: PlayAffectedEnemyVfx: (%s,%s) - Setting enemy radius parameter Parameter: %s -> %fm"),
			*GetName(), *ActivationVfx->GetName(), *NiagaraComp->GetName(), *EnemyRadiusName.ToString(), EnemyRadius / 100);
	}

	UE_VLOG_UELOG(GetOwner(), LogTRItem, Log, TEXT("%s: PlayAffectedEnemyVfx: Could not play %s for Enemy=%s"), *GetName(), *ActivationVfx->GetName(), *Enemy->GetName());

	return NiagaraComp;
}

#pragma endregion Niagara Vfx
