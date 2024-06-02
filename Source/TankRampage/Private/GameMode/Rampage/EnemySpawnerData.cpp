#include "EnemySpawnerData.h"

#include "Utils/TRDataTableUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnemySpawnerData)

TArray<FEnemySpawnerData> EnemySpawnerDataParser::ReadAll(UDataTable* EnemySpawnerDataTable)
{
	if (!TR::DataTableUtils::ValidateDataTableRowType<FEnemySpawnerData>(EnemySpawnerDataTable))
	{
		return {};
	}

	TArray<FEnemySpawnerData*> Data;
	EnemySpawnerDataTable->GetAllRows("EnemySpawnerDataParser::ReadAll", Data);

	TArray<FEnemySpawnerData> OutputData;
	OutputData.Reserve(Data.Num());

	for (auto DataRowPtr : Data)
	{
		if (DataRowPtr)
		{
			OutputData.Add(*DataRowPtr);
		}
	}

	return OutputData;
}
