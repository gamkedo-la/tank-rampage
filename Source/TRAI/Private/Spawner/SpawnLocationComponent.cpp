// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner/SpawnLocationComponent.h"

#include "Components/BillboardComponent.h"

// Sets default values for this component's properties
USpawnLocationComponent::USpawnLocationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bHiddenInGame = true;
}
