// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "LevelUnlocksContext.h"

#include "LevelUnlocksData.generated.h"

class UDataTable;

namespace LevelUnlocksParser
{
    TArray<FLevelUnlocksConfig> ToConfigArray(UDataTable* LevelUnlocksDataTable,
        UDataTable* LevelUpDataTable);

    TArray<int32> ToXPLevelRequirementsArray(UDataTable* LevelUpDataTable);
}
/**
 * 
 */
USTRUCT(BlueprintType)
struct FLevelUnlocksData : public FTableRowBase
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Player Level")
    int32 PlayerLevel{ 1 };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Description")
    FString Description{};

    UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Item Name")
    FName ItemName{};

    UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Item Level")
    int32 ItemLevel{ 1 };
};

USTRUCT(BlueprintType)
struct FLevelUpData : public FTableRowBase
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "XP")
    int32 XP{};

    UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Unlock Choices")
    int32 UnlockChoices{ 3 };

};
