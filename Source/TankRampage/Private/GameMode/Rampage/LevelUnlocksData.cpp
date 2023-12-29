// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/LevelUnlocksData.h"

#include "Logging/LoggingUtils.h"
#include "TankRampageLogging.h"

namespace
{
    constexpr int32 FirstLevelUnlockCount = 1;
    constexpr int32 DefaultLevelUnlockCount = 3;

    TArray<FLevelUpData*> ParseLevelUnlockOptions(UDataTable* LevelUpDataTable);
    TMap<int32, TArray<FLevelUnlock>> ParseLevelUnlocksByPlayerLevel(UDataTable* LevelUnlocksDataTable);
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

    TMap<int32, TArray<FLevelUnlock>> LevelUnlocksByPlayerLevel = ParseLevelUnlocksByPlayerLevel(LevelUnlocksDataTable);
    // Starts at Level 2
    TArray<FLevelUpData*> LevelUnlockOptions = ParseLevelUnlockOptions(LevelUpDataTable);

    if (LevelUnlockOptions.Num() + 1 != LevelUnlocksByPlayerLevel.Num())
    {
        UE_LOG(LogTankRampage, Warning,
            TEXT("LevelUnlocksParser::ToConfigArray - LevelUpDataTable=%s number of rows=%d (starting at level 2) does not match LevelUnlocksDataTable=%s player levels count=%d (starting at level 1)"),
            *LevelUpDataTable->GetName(), LevelUnlockOptions.Num(), *LevelUnlocksDataTable->GetName(), LevelUnlocksByPlayerLevel.Num()
        );
    }

    TArray<FLevelUnlocksConfig> LevelUnlocksConfig;
    LevelUnlocksConfig.Reserve(LevelUnlocksByPlayerLevel.Num());

    for(int32 PlayerLevel = 1; PlayerLevel <= LevelUnlocksByPlayerLevel.Num(); ++PlayerLevel)
    {
        TArray<FLevelUnlock>* LevelUnlocksFindResult = LevelUnlocksByPlayerLevel.Find(PlayerLevel);
        if (LevelUnlocksFindResult)
        {
            const auto LevelOptionCount = GetLevelOptionsByPlayerLevel(LevelUnlockOptions, PlayerLevel);

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
                .MaxUnlockOptions = 0
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
        LevelUpDataTable->GetAllRows("ParseLevelUnlockOptions", Data);

        return Data;
    }

    TMap<int32, TArray<FLevelUnlock>> ParseLevelUnlocksByPlayerLevel(UDataTable* LevelUnlocksDataTable)
    {
        TMap<int32, TArray<FLevelUnlock>> ParsedData;

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