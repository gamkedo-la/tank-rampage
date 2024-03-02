// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/AttributeModifierEffect.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/TRGameplayTags.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

bool UAttributeModifierEffect::DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName)
{
	const FGameplayTagContainer BuffTagsContainer = FGameplayTagContainer::CreateFromArray(BuffTags);

	UE_VLOG_UELOG(GetOwner(), LogTRItem, Log, TEXT("%s-%s: DoActivation: Speed multiplier=%f; BuffTags=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), AttributeMultiplier, *BuffTagsContainer.ToStringSimple());

	if (!ApplyEffectToSelf(BuffTagsContainer))
	{
		return false;
	}

	ScheduleEffectRemoval();

	// TODO: Play SFX and VFX

	return true;
}

void UAttributeModifierEffect::BeginDestroy()
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

void UAttributeModifierEffect::ScheduleEffectRemoval()
{
	auto World = GetWorld();
	check(World);

	// If previous effect is still active, the new multiplier will replace the previous - it always increases so this will not negatively effect the player
	World->GetTimerManager().SetTimer(TagExpirationHandle, this, &ThisClass::RemoveEffect, DurationSeconds, false);
}

void UAttributeModifierEffect::RemoveEffect()
{
	const FGameplayTagContainer BuffTagsContainer = FGameplayTagContainer::CreateFromArray(BuffTags);

	UE_VLOG_UELOG(GetOwner(), LogTRItem, Log, TEXT("%s-%s: RemoveEffect: BuffTags=%s; LastTagCount=%d"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *BuffTagsContainer.ToStringSimple(), LastTagCount);

	if (auto ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()); ASC)
	{
		ASC->RemoveLooseGameplayTags(BuffTagsContainer, LastTagCount);
	}

	LastTagCount = 0;
}

bool UAttributeModifierEffect::ApplyEffectToSelf(const FGameplayTagContainer& TagsContainer)
{
	auto ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());

	if (!ensureMsgf(ASC, TEXT("Owner=%s is not configured with an AbilitySystemComponent"), *LoggingUtils::GetName(GetOwner())))
	{
		return false;
	}

	if (!ensureMsgf(!TagsContainer.IsEmpty(), TEXT("BuffTags not configured!")))
	{
		return false;
	}

	// TODO: Do this from a gameplay effect
	LastTagCount = TR::GameplayTags::AttributeMultiplierValueToTagCount(AttributeMultiplier);
	ASC->AddLooseGameplayTags(TagsContainer, LastTagCount);

	return true;
}
