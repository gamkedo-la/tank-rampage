// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Utils/StringUtils.h"

#include <compare>
#include <tuple>

#include "LevelUnlocksContext.generated.h"

class UItem;

USTRUCT(BlueprintType)
struct FLevelUnlock
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Description{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName ItemName{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Level{};
};

auto operator<=>(const FLevelUnlock& First, const FLevelUnlock& Second);
bool operator==(const FLevelUnlock& First, const FLevelUnlock& Second);
bool operator!=(const FLevelUnlock& First, const FLevelUnlock& Second);

// For TSet, TMap
int32 GetTypeHash(const FLevelUnlock& Key);

USTRUCT(BlueprintType)
struct FLevelUnlocksConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "Description"))
	TArray<FLevelUnlock> AvailableUnlocks;

	UPROPERTY(EditDefaultsOnly)
	int32 MaxUnlockOptions{ };

	operator bool() const;
};

USTRUCT(BlueprintType)
struct FLevelUnlocksContext
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FLevelUnlocksConfig Config;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	APawn* Pawn{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Level{};
};


#pragma region Inline Definitions
inline auto operator<=>(const FLevelUnlock& First, const FLevelUnlock& Second)
{
	return std::tie(First.Level, First.ItemName) <=> std::tie(Second.Level, Second.ItemName);
}

inline bool operator==(const FLevelUnlock& First, const FLevelUnlock& Second)
{
	return First.Level == Second.Level && First.ItemName == Second.ItemName;
}

inline bool operator!=(const FLevelUnlock& First, const FLevelUnlock& Second)
{
	return !(First == Second);
}

inline FLevelUnlocksConfig::operator bool() const
{
	return !AvailableUnlocks.IsEmpty();
}

inline int32 GetTypeHash(const FLevelUnlock& Key)
{
	return HashCombine(GetTypeHash(Key.Level), GetTypeHash(Key.ItemName));
}

#pragma endregion Inline Definitions

