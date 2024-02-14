// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"
#include "Item/ItemConfigData.h"
#include "Item/ItemSubsystem.h"

#include "AbilitySystem/TRGameplayTags.h"

bool UItem::CanBeActivated() const
{
	auto World = GetWorld();
	check(World);

	// Check if owning actor has a debuff to block item activations
	if (TR::GameplayTags::HasExactTag(GetOwner(), TR::GameplayTags::ItemBlocked))
	{
		UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s-%s: CanBeActivated: Item usage blocked by GameplayTag=%s"),
			*LoggingUtils::GetName(GetOuter()), *GetName(), *TR::GameplayTags::ItemBlocked.ToString());

		return false;
	}

	if (LastActivationTimeSeconds < 0)
	{
		return true;
	}

	return World->GetTimeSeconds() - LastActivationTimeSeconds > CooldownTimeSeconds;
}

float UItem::GetCooldownTimeRemaining() const
{
	UWorld* World = GetWorld();
	check(World);

	float CurrentTime = World->GetTimeSeconds();
	float TimeElapsedSinceLastFired = CurrentTime - LastActivationTimeSeconds;
	bool bIsFireOnCooldown = TimeElapsedSinceLastFired < CooldownTimeSeconds;

	return bIsFireOnCooldown ? CooldownTimeSeconds - TimeElapsedSinceLastFired : 0.f;
}

float UItem::GetCooldownProgressPercentage() const
{
	UWorld* World = GetWorld();
	check(World);

	float CurrentTime = World->GetTimeSeconds();
	float TimeElapsedSinceLastFired = CurrentTime - LastActivationTimeSeconds;
	bool bIsFireOnCooldown = TimeElapsedSinceLastFired < CooldownTimeSeconds;
	float ProgressPercentage = TimeElapsedSinceLastFired / CooldownTimeSeconds;
	return bIsFireOnCooldown ? ProgressPercentage : 1.f;
}

bool UItem::Activate(USceneComponent* ActivationReferenceComponent, const FName& ActivationSocketName)
{
	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s-%s: Activate: ActivationReferenceComponent=%s; ActivationSocketName=%s"),
		*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(ActivationReferenceComponent), *ActivationSocketName.ToString());

	auto World = GetWorld();
	if (!World)
	{
		UE_VLOG_UELOG(GetOuter(), LogTRItem, Error, TEXT("%s-%s: Activate: World is NULL"), *LoggingUtils::GetName(GetOuter()), *GetName());
		return false;
	}

	if (!ActivationReferenceComponent)
	{
		UE_VLOG_UELOG(GetOuter(), LogTRItem, Error, TEXT("%s-%s: ActivationReferenceComponent is NULL"), *LoggingUtils::GetName(GetOuter()), *GetName());
		return false;
	}

	if(!CanBeActivated())
	{
		return false;
	}

	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s-%s: DoActivation"), *LoggingUtils::GetName(GetOuter()), *GetName());

	if (!DoActivation(*ActivationReferenceComponent, ActivationSocketName))
	{
		return false;
	}

	LastActivationTimeSeconds = World->GetTimeSeconds();

	return true;
}

void UItem::Initialize(APawn* InOwner, const FItemConfigData& ItemConfigData)
{
	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s-%s: Initialize: InOwner=%s; ItemConfigData=%s"),
		*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(InOwner), *ItemConfigData.ToString());

	Owner = InOwner;

	NativeInitialize(ItemConfigData);
	BlueprintInitialize(ItemConfigData);
}

void UItem::SetLevel(int32 Level)
{
	const auto PreviousLevel = ItemLevel;

	ItemLevel = FMath::Min(Level, MaxItemLevel);

	if (PreviousLevel != ItemLevel)
	{
		BeforeOnLevelChanged(ItemLevel, PreviousLevel);

		OnLevelChanged(ItemLevel, PreviousLevel);

		auto World = GetWorld();
		check(World);

		if (auto ItemSubsystem = World->GetSubsystem<UItemSubsystem>(); ensure(ItemSubsystem))
		{
			ItemSubsystem->OnItemUpgraded.Broadcast(this);
		}

		AfterOnLevelChanged(ItemLevel, PreviousLevel);
	}
}
