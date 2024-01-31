#include "AbilitySystem/TRGameplayTags.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

TRITEM_API bool TR::GameplayTags::HasExactTag(AActor* Actor, const FName& TagName)
{
	if (!Actor)
	{
		return false;
	}

	auto AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (!AbilitySystemComponent)
	{
		return false;
	}

	// Cache the FName -> FGameplayTag mappings
	thread_local TMap<FName, FGameplayTag> TagMap;

	FGameplayTag* Tag = TagMap.Find(TagName);
	if (!Tag)
	{
		Tag = &TagMap.Add(TagName, FGameplayTag::RequestGameplayTag(TagName));
	}

	return AbilitySystemComponent->HasMatchingGameplayTag(*Tag);
}
