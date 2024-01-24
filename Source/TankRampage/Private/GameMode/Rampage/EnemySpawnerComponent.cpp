// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/EnemySpawnerComponent.h"

#include "Spawner/EnemySpawner.h"

#include "TankRampageLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

UEnemySpawnerComponent::UEnemySpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UEnemySpawnerComponent::BeginPlay()
{
	Super::BeginPlay();

	// spawning frequency will be determined by a data table based on time elapsed 
	// the component will run on a timer and determine if new enemies need to spawn and select the most relevant spawners
	// This will be based on those at the appropriate distance to player (determined by the spawner relevancy bounds) and 
	// how many enemies should be spawned within the spawn group of the selected spawner so that clusters of enemies can be spawned easily

	InitSpawners();

	UE_VLOG_UELOG(GetOwner(), LogTankRampage, Log, TEXT("%s-%s: BeginPlay - Found %d enemy spawners"), *LoggingUtils::GetName(GetOwner()), *GetName(), Spawners.Num());
}

void UEnemySpawnerComponent::InitSpawners()
{
	auto World = GetWorld();
	check(World);

	Spawners.Reset();

	for (TObjectIterator<AEnemySpawner> It; It; ++It)
	{
		if (World != It->GetWorld())
		{
			continue;
		}

		Spawners.Add(*It);
	}
}
