// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataTable.h"

#include <concepts>

namespace TR::DataTableUtils
{
	template<std::derived_from<FTableRowBase> T>
	bool ValidateDataTableRowType(UDataTable* DataTable);
}


#pragma region Inline Definitions


template<std::derived_from<FTableRowBase> T>
inline bool TR::DataTableUtils::ValidateDataTableRowType(UDataTable* DataTable)
{
    if (!ensureAlwaysMsgf(DataTable, TEXT("DataTable was NULL for expected row type=%s"), *T::StaticStruct()->GetFName().ToString()))
    {
        return false;
    }

    // GetRowStructPathName is only available in editor builds
#if WITH_EDITOR
    return ensureAlwaysMsgf(DataTable->GetRowStruct()->IsChildOf(T::StaticStruct()), TEXT("DataTable=%s has row type %s where %s was expected: AssetPath=%s"),
        *DataTable->GetName(), *DataTable->GetRowStructPathName().ToString(), *T::StaticStruct()->GetFName().ToString(), *DataTable->GetPathName());
#else
    return ensureAlwaysMsgf(DataTable->GetRowStruct()->IsChildOf(T::StaticStruct()), TEXT("DataTable=%s expected row type=%s for AssetPath=%s"),
        *DataTable->GetName(), *T::StaticStruct()->GetFName().ToString(), *DataTable->GetPathName());
#endif
}

#pragma endregion Inline Definitions
