// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "LevelUnlocksContext.generated.h"

class UItem;

USTRUCT(BlueprintType)
struct FLevelUnlock
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString Description{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName ItemName{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Level{};
};

bool operator == (const FLevelUnlock& First, const FLevelUnlock& Second);
bool operator != (const FLevelUnlock& First, const FLevelUnlock& Second);

USTRUCT(BlueprintType)
struct FLevelUnlocksConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "Description"))
	TArray<FLevelUnlock> AvailableUnlocks;

	UPROPERTY(EditDefaultsOnly)
	int32 MaxUnlockOptions{ 3 };

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
inline bool operator == (const FLevelUnlock& First, const FLevelUnlock& Second)
{
	return First.Level == Second.Level && First.ItemName == Second.ItemName;
}

inline bool operator != (const FLevelUnlock& First, const FLevelUnlock& Second)
{
	return !(First == Second);
}

inline FLevelUnlocksConfig::operator bool() const
{
	return !AvailableUnlocks.IsEmpty();
}

#pragma endregion Inline Definitions

