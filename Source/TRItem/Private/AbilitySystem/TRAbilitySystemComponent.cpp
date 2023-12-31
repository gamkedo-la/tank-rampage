// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/TRAbilitySystemComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

void UTRAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	UE_VLOG_UELOG(GetOwner(), LogTRItem, Log, TEXT("%s-%s: InitAbilityActorInfo - InOwnerActor=%s; InAvatarActor=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(InOwnerActor), *LoggingUtils::GetName(InAvatarActor));

	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (!EffectAppliedDelegateHandle.IsValid())
	{
		EffectAppliedDelegateHandle = OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &ThisClass::EffectApplied);
	}
}

void UTRAbilitySystemComponent::EffectApplied(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& GameplayEffectSpec, FActiveGameplayEffectHandle ActiveGameplayEffectHandle)
{
	FGameplayTagContainer GameplayTagContainer;
	GameplayEffectSpec.GetAllAssetTags(GameplayTagContainer);

	UE_VLOG_UELOG(GetOwner(), LogTRItem, Log, TEXT("%s-%s: EffectApplied - GameplayEffectSpec=%s; AssetTags=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *GameplayEffectSpec.ToSimpleString(), *GameplayTagContainer.ToStringSimple());

	// Broadcast any gameplay tags the effect has which the HUD could listen for and respond to
	OnEffectAssetTagsAdded.Broadcast(GameplayTagContainer);
}

#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void UTRAbilitySystemComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Ability System Component");

	Snapshot->Status.Add(Category);
}

#endif

#pragma endregion Visual Logger
