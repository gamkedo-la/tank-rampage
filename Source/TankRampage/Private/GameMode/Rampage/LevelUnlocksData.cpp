// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/LevelUnlocksData.h"

#include "Logging/LoggingUtils.h"
#include "TankRampageLogging.h"
#include "Utils/TRDataTableUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LevelUnlocksData)

namespace
{
    constexpr int32 FirstLevelUnlockCount = 1;
    constexpr int32 DefaultLevelUnlockCount = 3;

    TArray<FLevelUpData*> ParseLevelUnlockOptions(UDataTable* LevelUpDataTable);
    TSortedMap<int32, TArray<FLevelUnlock>> ParseLevelUnlocksByPlayerLevel(UDataTable* LevelUnlocksDataTable);
    int32 GetLevelOptionsByPlayerLevel(const TArray<FLevelUpData*>& LevelUpRows, int32 PlayerLevel);
}

TArray<FLevelUnlocksConfig> LevelUnlocksParser::ToConfigArray(UDataTable* LevelUnlocksDataTable,
    UDataTable* LevelUpDataTable) 
{
    if (!ensure(LevelUnlocksDataTable))
    {
        return {};
    }
    if (!ensure(LevelUpDataTable))
    {
        return {};
    }

    TSortedMap<int32, TArray<FLevelUnlock>> LevelUnlocksByPlayerLevel = ParseLevelUnlocksByPlayerLevel(LevelUnlocksDataTable);

    const int32 MaxUnlockLevel = [&LevelUnlocksByPlayerLevel]()
    {
         // Get the last element which will have the max level
         decltype(LevelUnlocksByPlayerLevel)::TConstReverseIterator RevIt(LevelUnlocksByPlayerLevel);
         return RevIt ? RevIt.Key() : 0;
    }();

    // Starts at Level 2
    TArray<FLevelUpData*> LevelUnlockOptions = ParseLevelUnlockOptions(LevelUpDataTable);

    if (LevelUnlockOptions.Num() + 1 < MaxUnlockLevel)
    {
        UE_LOG(LogTankRampage, Warning,
            TEXT("LevelUnlocksParser::ToConfigArray - LevelUpDataTable=%s number of rows=%d (starting at level 2) is fewer than LevelUnlocksDataTable=%s player levels count=%d (starting at level 1)"),
            *LevelUpDataTable->GetName(), LevelUnlockOptions.Num(), *LevelUnlocksDataTable->GetName(), MaxUnlockLevel
        );
    }

    TArray<FLevelUnlocksConfig> LevelUnlocksConfig;
    LevelUnlocksConfig.Reserve(MaxUnlockLevel);

    for(int32 PlayerLevel = 1; PlayerLevel <= MaxUnlockLevel; ++PlayerLevel)
    {
        TArray<FLevelUnlock>* LevelUnlocksFindResult = LevelUnlocksByPlayerLevel.Find(PlayerLevel);
        const auto LevelOptionCount = GetLevelOptionsByPlayerLevel(LevelUnlockOptions, PlayerLevel);

        if (LevelUnlocksFindResult)
        {
            LevelUnlocksConfig.Add(FLevelUnlocksConfig
            {
                .AvailableUnlocks = std::move(*LevelUnlocksFindResult),
                .MaxUnlockOptions = LevelOptionCount
            });
        }
        else
        {
            UE_LOG(LogTankRampage, Warning,
                TEXT("LevelUnlocksParser::ToConfigArray - LevelUnlocksDataTable=%s does not have an entry for PlayerLevel=%d"),
                *LevelUnlocksDataTable->GetName(), PlayerLevel
            );

            LevelUnlocksConfig.Add(FLevelUnlocksConfig
            {
                .AvailableUnlocks = {},
                .MaxUnlockOptions = LevelOptionCount
            });
        }
    }

    return LevelUnlocksConfig;
}

TArray<int32> LevelUnlocksParser::ToXPLevelRequirementsArray(UDataTable* LevelUpDataTable)
{
    if (!ensure(LevelUpDataTable))
    {
        return {};
    }

    TArray<FLevelUpData*> ParsedLevelUpData = ParseLevelUnlockOptions(LevelUpDataTable);

    TArray<int32> XPLevels;
    XPLevels.Reserve(ParsedLevelUpData.Num());

    for (auto LevelUpDataPtr : ParsedLevelUpData)
    {
        if (LevelUpDataPtr)
        {
            XPLevels.Add(LevelUpDataPtr->XP);
        }
    }

    return XPLevels;
}

namespace
{
    inline TArray<FLevelUpData*> ParseLevelUnlockOptions(UDataTable* LevelUpDataTable)
    {
        TArray<FLevelUpData*> Data;

        if (TR::DataTableUtils::ValidateDataTableRowType<FLevelUpData>(LevelUpDataTable))
        {
            LevelUpDataTable->GetAllRows("ParseLevelUnlockOptions", Data);
        }

        return Data;
    }

    TSortedMap<int32, TArray<FLevelUnlock>> ParseLevelUnlocksByPlayerLevel(UDataTable* LevelUnlocksDataTable)
    {
        if (!TR::DataTableUtils::ValidateDataTableRowType<FLevelUnlocksData>(LevelUnlocksDataTable))
        {
            return {};
        }

        TSortedMap<int32, TArray<FLevelUnlock>> ParsedData;

        for (const auto& [_, RowData] : LevelUnlocksDataTable->GetRowMap())
        {
            auto RowPtr = reinterpret_cast<FLevelUnlocksData*>(RowData);
            if (!RowPtr)
            {
                continue;
            }

            auto& LevelArray = ParsedData.FindOrAdd(RowPtr->PlayerLevel);
            LevelArray.Add(FLevelUnlock{
                .Description = RowPtr->Description,
                .ItemName = RowPtr->ItemName,
                .Level = RowPtr->ItemLevel
             });
        }

        return ParsedData;
    }

    int32 GetLevelOptionsByPlayerLevel(const TArray<FLevelUpData*>& LevelUpRows, int32 PlayerLevel)
    {
        // automatic unlock of first item
        // PlayerLevel <= 1
        if (PlayerLevel == 1 || !ensureMsgf(PlayerLevel > 0, TEXT("PlayerLevel=%d <= 0"), PlayerLevel))
        {
            return FirstLevelUnlockCount;
        }

        if (LevelUpRows.IsEmpty())
        {
            UE_LOG(LogTankRampage, Warning,
                TEXT("LevelUnlocksParser::GetLevelOptionsByPlayerLevel - LevelUpDataTable does not contain any rows!")
            );

            return DefaultLevelUnlockCount;
        }

        // PlayerLevel starts at 1
        // First entry is Level 2
        const auto Index = FMath::Min(PlayerLevel - 2, LevelUpRows.Num() - 1);
        auto Row = LevelUpRows[Index];
        if (!Row)
        {
            UE_LOG(LogTankRampage, Error,
                TEXT("LevelUnlocksParser::GetLevelOptionsByPlayerLevel - LevelUpRows Index=%d has a NULL row!"),
                Index
            );

            return DefaultLevelUnlockCount;
        }

        return Row->UnlockChoices;
    }
}