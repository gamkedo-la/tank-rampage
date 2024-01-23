// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner/EnemySpawnerGroup.h"

#include "Spawner/EnemySpawner.h"

#include "Logging/LoggingUtils.h"
#include "TRAILogging.h"
#include "VisualLogger/VisualLogger.h"


AEnemySpawnerGroup::AEnemySpawnerGroup()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEnemySpawnerGroup::Initialize(UPrimitiveComponent* InOverlapComponent)
{
	if (!ensure(InOverlapComponent))
	{
		return;
	}

	TSet<AActor*> OverlappingActors;
	InOverlapComponent->GetOverlappingActors(OverlappingActors, AEnemySpawner::StaticClass());

	Spawners.Reset(OverlappingActors.Num());

	for (auto Actor : OverlappingActors)
	{
		if (auto Spawner = Cast<AEnemySpawner>(Actor); Spawner)
		{
			Spawners.Add(Spawner);
		}
	}

	UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s: Initialize - %d spawners"), *GetName(), Spawners.Num());
}

