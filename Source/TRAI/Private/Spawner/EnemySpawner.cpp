// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner/EnemySpawner.h"

#include "Logging/LoggingUtils.h"
#include "TRAILogging.h"
#include "VisualLogger/VisualLogger.h"

#include <limits>

AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Spawn Point"));
}

APawn* AEnemySpawner::Spawn()
{
	const auto SpawnClass = SelectSpawnClass();
	if (!SpawnClass)
	{
		return nullptr;
	}

	auto World = GetWorld();
	check(World);

	FActorSpawnParameters Params;

	auto Spawned = World->SpawnActor<APawn>(SpawnClass, GetActorLocation(), FRotator::ZeroRotator, Params);
	if (Spawned)
	{
		UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s: Spawn - Spawned %s -> %s"), *GetName(), *SpawnClass->GetName(), *Spawned->GetName());
		UE_VLOG_LOCATION(this, LogTRAI, Log, GetActorLocation(), 50.0f, FColor::Green, TEXT("Spawn %s"), *SpawnClass->GetName());
		LastSpawnTime = World->GetTimeSeconds();
	}
	else
	{
		UE_VLOG_UELOG(this, LogTRAI, Warning, TEXT("%s: Spawn - Failed to Spawn %s"), *GetName(), *SpawnClass->GetName());
		UE_VLOG_LOCATION(this, LogTRAI, Warning, GetActorLocation(), 50.0f, FColor::Red, TEXT("Failed to spawn %s"), *SpawnClass->GetName());
	}

	return Spawned;
}

float AEnemySpawner::GetTimeSinceLastSpawn() const
{
	auto World = GetWorld();
	if (!World || LastSpawnTime < 0)
	{
		return std::numeric_limits<float>::max();
	}

	return World->GetTimeSeconds() - LastSpawnTime;
}

void AEnemySpawner::BeginPlay()
{
	UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();

	RegisterOverlaps();
}

void AEnemySpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	DeregisterOverlaps();
}

void AEnemySpawner::Initialize(UPrimitiveComponent* InOverlapComponent)
{
	UE_VLOG_UELOG(this, LogTRAI, Log, TEXT("%s: Initialize - OverlapComponent=%s;SpawningArea=%s"), *GetName(), *LoggingUtils::GetName(InOverlapComponent));

	if (!ensure(InOverlapComponent))
	{
		return;
	}

	OverlapComponent = InOverlapComponent;
	RegisterOverlaps();
}

void AEnemySpawner::RegisterOverlaps()
{
	if (!OverlapComponent)
	{
		return;
	}

	OverlapComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapBegin);
	OverlapComponent->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapEnd);
}

void AEnemySpawner::DeregisterOverlaps()
{
	if (!OverlapComponent)
	{
		return;
	}

	OverlapComponent->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnOverlapBegin);
	OverlapComponent->OnComponentEndOverlap.RemoveDynamic(this, &ThisClass::OnOverlapEnd);
}

UClass* AEnemySpawner::SelectSpawnClass() const
{
	if (!ensureMsgf(!SpawningTypes.IsEmpty(), TEXT("%s: SelectSpawnClass - SpawningTypes is empty"), *GetName()))
	{
		return nullptr;
	}

	TArray<int32, TInlineAllocator<64>> ValidTypeIndices;
	for (int32 i = 0; i < SpawningTypes.Num(); ++i)
	{
		if (!CanSpawnEnemy.IsBound() || CanSpawnEnemy.Execute(SpawningTypes[i]))
		{
			ValidTypeIndices.Add(i);
		}
	}

	if (ValidTypeIndices.IsEmpty())
	{
		UE_VLOG_UELOG(this, LogTRAI, Display, TEXT("%s: SelectSpawnClass - could not find an available spawning type from the %d available"), *GetName(), SpawningTypes.Num());
		return nullptr;
	}

	const int32 SelectedValidIndex = FMath::RandRange(0, ValidTypeIndices.Num() - 1);
	check(SelectedValidIndex < ValidTypeIndices.Num());

	const int32 SelectedIndex = ValidTypeIndices[SelectedValidIndex];
	check(SelectedIndex >= 0 && SelectedIndex < SpawningTypes.Num());

	return SpawningTypes[SelectedIndex];
}

void AEnemySpawner::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// TODO:
}

void AEnemySpawner::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// TODO:
}
