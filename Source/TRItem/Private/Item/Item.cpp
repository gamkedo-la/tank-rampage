// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Item.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"
#include "Item/ItemConfigData.h"
#include "Item/ItemSubsystem.h"

#include "AbilitySystem/TRGameplayTags.h"

#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Item)

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

	// Never activated or no cooldown
	if (LastActivationTimeSeconds < 0 || CooldownTimeSeconds <= 0)
	{
		return 0.0f;
	}

	const auto CurrentTime = World->GetTimeSeconds();
	const auto TimeElapsedSinceLastFired = CurrentTime - LastActivationTimeSeconds;

	if (TimeElapsedSinceLastFired >= CooldownTimeSeconds)
	{
		return 0.0f;
	}

	return CooldownTimeSeconds - TimeElapsedSinceLastFired;
}

float UItem::GetCooldownProgressPercentage() const
{
	UWorld* World = GetWorld();
	check(World);

	// Never activated or no cooldown
	if (LastActivationTimeSeconds < 0 || CooldownTimeSeconds <= 0)
	{
		return 1.0f;
	}

	const auto CurrentTime = World->GetTimeSeconds();
	const auto TimeElapsedSinceLastFired = CurrentTime - LastActivationTimeSeconds;

	if (TimeElapsedSinceLastFired >= CooldownTimeSeconds)
	{
		return 1.0f;
	}

	return TimeElapsedSinceLastFired / CooldownTimeSeconds;
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

	OnItemActivated.Broadcast(this);

	if (!DoActivation(*ActivationReferenceComponent, ActivationSocketName))
	{
		return false;
	}

	LastActivationTimeSeconds = World->GetTimeSeconds();

	RegisterCooldownTimer();

	return true;
}

bool UItem::ActivateOnRootComponent()
{
	auto OwningActor = GetOwner();

	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s-%s: ActivateOnRootComponent: OwningActor=%s"),
		*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(OwningActor));

	if (!OwningActor)
	{
		return false;
	}

	return Activate(OwningActor->GetRootComponent(), NAME_None);
}

void UItem::Initialize(APawn* InOwner, const FItemConfigData& ItemConfigData)
{
	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s-%s: Initialize: InOwner=%s; ItemConfigData=%s"),
		*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(InOwner), *ItemConfigData.ToString());

	Owner = InOwner;

	NativeInitialize(ItemConfigData);
	BlueprintInitialize(ItemConfigData);
}

void UItem::BeginDestroy()
{
	Super::BeginDestroy();

	UnregisterCooldownTimer();
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

FString UItem::ToString() const
{
	FString s;

	NativeToString(s);
	s += BlueprintToString();

	return s;
}

void UItem::NativeToString(FString& str) const
{
	str.Appendf(TEXT("Level %d; CooldownTime=%.1fs"), GetLevel(), GetCooldownTimeRemaining());
}

#pragma region Cooldown Notify

void UItem::RegisterCooldownTimer()
{
	if (!bRequestsCooldownNotify || CooldownTimeSeconds <= 0)
	{
		return;
	}

	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s-%s: RegisterCooldownTimer: CooldownTime=%fs"),
		*LoggingUtils::GetName(GetOuter()), *GetName(), CooldownTimeSeconds);

	auto World = GetWorld();
	check(World);

	World->GetTimerManager().SetTimer(CooldownTimerHandle, this, &ThisClass::NotifyCooldownComplete, CooldownTimeSeconds);
}

void UItem::UnregisterCooldownTimer()
{
	if (!bRequestsCooldownNotify)
	{
		return;
	}

	if (auto World = GetWorld(); World)
	{
		World->GetTimerManager().ClearTimer(CooldownTimerHandle);
	}
	else
	{
		CooldownTimerHandle.Invalidate();
	}
}

void UItem::NotifyCooldownComplete()
{
	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s-%s: NotifyCooldownComplete"),
		*LoggingUtils::GetName(GetOuter()), *GetName());

	OnCooldownComplete();
	BlueprintCooldownComplete();
}

#pragma endregion Cooldown Notify

#pragma region Audio

UAudioComponent* UItem::PlaySfxAtActorLocation(USoundBase* Sound) const
{
	if (!ensure(Sound))
	{
		return nullptr;
	}

	auto ComponentOwner = GetOwner();
	if (!ensure(ComponentOwner))
	{
		return nullptr;
	}

	// The owner of the audio component is derived from the world context object and this will control the sound concurrency
	auto SpawnedAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
		ComponentOwner,
		Sound,
		ComponentOwner->GetActorLocation(), ComponentOwner->GetActorRotation()
	);

	if (!SpawnedAudioComponent)
	{
		// This is not an error condition as the component may not spawn if the sound is not audible, for example it attenuates below a threshold based on distance
		UE_VLOG_UELOG(this, LogTRItem, Log,
			TEXT("%s-%s: PlaySfxAtActorLocation - Unable to spawn audio component for sfx=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Sound->GetName());
		return nullptr;
	}

	UE_VLOG_UELOG(this, LogTRItem, Log,
		TEXT("%s-%s: PlaySfxAtActorLocation - Playing sfx=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *Sound->GetName());

	SpawnedAudioComponent->bAutoDestroy = true;

	return SpawnedAudioComponent;
}

#pragma endregion Audio

UAudioComponent* UItem::PlaySfxAttached(USoundBase* Sound) const
{
	if (!ensure(Sound))
	{
		return nullptr;
	}

	auto ComponentOwner = GetOwner();
	if (!ensure(ComponentOwner))
	{
		return nullptr;
	}

	// The owner of the audio component is derived from the world context object and this will control the sound concurrency
	auto SpawnedAudioComponent = UGameplayStatics::SpawnSoundAttached(
		Sound,
		ComponentOwner->GetRootComponent(),
		NAME_None,
		FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset,
		true
	);

	if (!SpawnedAudioComponent)
	{
		// This is not an error condition as the component may not spawn if the sound is not audible, for example it attenuates below a threshold based on distance
		UE_VLOG_UELOG(this, LogTRItem, Log,
			TEXT("%s-%s: PlaySfxAttached - Unable to spawn audio component for sfx=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Sound->GetName());
		return nullptr;
	}

	UE_VLOG_UELOG(this, LogTRItem, Log,
		TEXT("%s-%s: PlaySfxAttached - Playing sfx=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *Sound->GetName());

	SpawnedAudioComponent->bAutoDestroy = true;

	return SpawnedAudioComponent;
}
