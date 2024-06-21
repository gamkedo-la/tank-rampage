#include "AbilitySystem/TRGameplayTags.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

namespace
{
	UAbilitySystemComponent* GetAbilitySystemComponent(AActor* Actor);
}

bool TR::GameplayTags::HasExactTag(AActor* Actor, const FName& TagName)
{
	auto AbilitySystemComponent = GetAbilitySystemComponent(Actor);
	if (!AbilitySystemComponent)
	{
		return false;
	}

	FGameplayTag* Tag = GetTagByName(TagName);
	check(Tag);

	return AbilitySystemComponent->HasMatchingGameplayTag(*Tag);
}

int32 TR::GameplayTags::GetExactTagCount(AActor* Actor, const FName& TagName)
{
	auto AbilitySystemComponent = GetAbilitySystemComponent(Actor);
	if (!AbilitySystemComponent)
	{
		return false;
	}

	FGameplayTag* Tag = GetTagByName(TagName);
	check(Tag);

	return AbilitySystemComponent->GetGameplayTagCount(*Tag);
}

float TR::GameplayTags::GetAttributeMultiplierFromTag(AActor* Actor, const FName& TagName)
{
	const int32 TagCount = GetExactTagCount(Actor, TagName);
	if (TagCount <= 0)
	{
		return 1.0f;
	}

	return AttributeMultiplierTagCountToValueFloat(TagCount);
}


FGameplayTag* TR::GameplayTags::GetTagByName(const FName& TagName)
{
	// Cache the FName -> FGameplayTag mappings
	thread_local TMap<FName, FGameplayTag> TagMap;

	FGameplayTag* Tag = TagMap.Find(TagName);
	if (!Tag)
	{
		Tag = &TagMap.Add(TagName, FGameplayTag::RequestGameplayTag(TagName));
	}

	return Tag;
}

namespace
{
	UAbilitySystemComponent* GetAbilitySystemComponent(AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return nullptr;
		}

		return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	}
}