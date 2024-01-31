// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/EMPWeapon.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"

#include <limits>

bool UEMPWeapon::DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName)
{
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
		auto ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy);
		// already checked in the sweep
		check(ASC);

		// TODO: Do this from a gameplay effect
		ASC->AddLooseGameplayTags(DebuffTagsContainer);

		AffectedActors.Add(ASC, EffectEndGameTime);
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

	DrawDebugSphere(GetWorld(), SweepLocation, Shape.GetSphereRadius(), 64, FColor::Blue, false, EffectDuration);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	TArray<FHitResult> HitResults;

	World->SweepMultiByChannel(HitResults, SweepLocation, SweepLocation + FVector{ 1 },
		FQuat::Identity, ECollisionChannel::ECC_Pawn, Shape, Params);

	TArray<APawn*> SelectedPawns;

	for (const auto& HitResult : HitResults)
	{
		auto Pawn = Cast<APawn>(HitResult.GetActor());
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

		// expired
		if (MapIt->Value <= CurrentTimeSeconds)
		{
			UE_VLOG_UELOG(GetOwner(), LogTRItem, Log, TEXT("%s-%s: CheckRemoveStunTags: Removing %s from %s"),
				*LoggingUtils::GetName(GetOwner()), *GetName(),
				*DebuffTagsContainer.ToString(), *LoggingUtils::GetName(ASC->GetOwner()));

			ASC->RemoveLooseGameplayTags(DebuffTagsContainer);
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
	for(auto [_, Time] : AffectedActors)
	{
		if (Time < MinTime)
		{
			MinTime = Time;
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
