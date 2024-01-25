// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "EnemySpawnerData.generated.h"

class UDataTable;

USTRUCT(BlueprintType)
struct FEnemySpawnerData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Spawn Count")
	int32 SpawnCount{ };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Spawn Cluster Size")
	int32 SpawnClusterSize{ };
};

namespace EnemySpawnerDataParser
{
	TArray<FEnemySpawnerData> ReadAll(UDataTable* EnemySpawnerDataTable);
}