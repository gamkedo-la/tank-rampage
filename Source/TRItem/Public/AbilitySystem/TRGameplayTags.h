#pragma once

#include "CoreMinimal.h"

struct FGameplayTag;

namespace TR::GameplayTags
{
	inline const FName MovementBlocked(TEXT("Debuff.Movement.Block"));
	inline const FName AimBlocked(TEXT("Debuff.Aim.Block"));
	inline const FName ItemBlocked(TEXT("Debuff.Item.Block"));
	inline const FName SpeedMultiplier(TEXT("Buff.Movement.Speed.Multiplier"));

	TRITEM_API bool HasExactTag(AActor* Actor, const FName& TagName);
	TRITEM_API int32 GetExactTagCount(AActor* Actor, const FName& TagName);
	TRITEM_API FGameplayTag* GetTagByName(const FName& TagName);

	/*
	* Gets the attribute multiplier corresponding to the given <c>TagName</c>. If no tags are applied, then <c>1</c> is returned.
	*/
	TRITEM_API float GetAttributeMultiplierFromTag(AActor* Actor, const FName& TagName);

	float AttributeMultiplierTagCountToValueFloat(int32 TagCount);
	int32 AttributeMultiplierValueToTagCount(float Multiplier);
}

#pragma region Inline Definitions

inline float TR::GameplayTags::AttributeMultiplierTagCountToValueFloat(int32 TagCount)
{
	return TagCount / 100.0f;
}

inline int32 TR::GameplayTags::AttributeMultiplierValueToTagCount(float Multiplier)
{
	return FMath::CeilToInt32(Multiplier * 100);
}

#pragma endregion Inline Definitions
