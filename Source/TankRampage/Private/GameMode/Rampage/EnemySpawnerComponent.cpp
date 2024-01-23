// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/EnemySpawnerComponent.h"

UEnemySpawnerComponent::UEnemySpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UEnemySpawnerComponent::BeginPlay()
{
	Super::BeginPlay();
}
