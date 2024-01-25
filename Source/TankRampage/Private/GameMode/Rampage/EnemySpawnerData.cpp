#include "EnemySpawnerData.h"

TArray<FEnemySpawnerData> EnemySpawnerDataParser::ReadAll(UDataTable* EnemySpawnerDataTable)
{
	if (!ensure(EnemySpawnerDataTable))
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
