// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/RampageGameMode.h"

#include "XPSpawnerComponent.h"

ARampageGameMode::ARampageGameMode()
{
	XPSpawnerComponent = CreateDefaultSubobject<UXPSpawnerComponent>(TEXT("XP Spawner"));
}
