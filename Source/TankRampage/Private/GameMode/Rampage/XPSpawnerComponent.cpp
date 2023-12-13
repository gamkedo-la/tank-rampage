// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/XPSpawnerComponent.h"

#include "Subsystems/TankEventsSubsystem.h"
#include "XPToken.h"
#include "Pawn/BaseTankPawn.h"

UXPSpawnerComponent::UXPSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UXPSpawnerComponent::BeginPlay()
{
	Super::BeginPlay();

	// TODO: Register tank destroyed event
	// TODO: If tank destroyed was not a player tank and destroyed by the player, then spawn the XP token of given item class at the location where the tank was killed (tank actor location)
}

