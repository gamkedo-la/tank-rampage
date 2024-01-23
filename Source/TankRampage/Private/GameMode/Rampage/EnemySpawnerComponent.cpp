// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/EnemySpawnerComponent.h"

UEnemySpawnerComponent::UEnemySpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UEnemySpawnerComponent::BeginPlay()
{
	Super::BeginPlay();

	// TODO: Find all the enemy spawners and spawner groups in the world
	// spawning frequency will be determined by a data table based on time elapsed 
	// the component will run on a timer and determine if new enemies need to spawn and select the most relevant spawners
	// This will be based on those at the appropriate distance to player (determined by the spawner relevancy bounds) and 
	// how many enemies should be spawned within the spawn group of the selected spawner so that clusters of enemies can be spawned easily
}
