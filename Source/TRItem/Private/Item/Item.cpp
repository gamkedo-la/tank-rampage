// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "Item/ItemDataAsset.h"

bool UItem::CanBeActivated() const
{
	auto World = GetWorld();
	check(World);

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
	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: Activate: ActivationReferenceComponent=%s; ActivationSocketName=%s"),
		*GetName(), *LoggingUtils::GetName(ActivationReferenceComponent), *ActivationSocketName.ToString());

	auto World = GetWorld();
	if (!World)
	{
		UE_VLOG_UELOG(this, LogTRItem, Error, TEXT("%s: Activate: World is NULL"), *GetName());
		return false;
	}

	if (!ActivationReferenceComponent)
	{
		UE_VLOG_UELOG(this, LogTRItem, Error, TEXT("%s: ActivationReferenceComponent is NULL"), *GetName());
		return false;
	}

	if(!CanBeActivated())
	{
		return false;
	}

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: DoActivation"), *GetName());

	if (!DoActivation(*ActivationReferenceComponent, ActivationSocketName))
	{
		return false;
	}

	LastActivationTimeSeconds = World->GetTimeSeconds();

	return true;
}

void UItem::Initialize(APawn* InOwner, UItemDataAsset* InItemDataAsset)
{
	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: Initialize: InOwner=%s; InItemDataAsset=%s"),
		*GetName(), *LoggingUtils::GetName(InOwner), *LoggingUtils::GetName(InItemDataAsset));

	check(InItemDataAsset);

	Owner = InOwner;

	NativeInitialize(InItemDataAsset);
	BlueprintInitialize(InItemDataAsset);
}

void UItem::SetLevel(int32 Level)
{
	const auto PreviousLevel = ItemLevel;

	ItemLevel = FMath::Min(Level, MaxItemLevel);

	if (PreviousLevel != ItemLevel)
	{
		OnLevelChanged(ItemLevel, PreviousLevel);
	}
}
