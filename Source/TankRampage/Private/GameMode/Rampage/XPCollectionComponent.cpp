// Fill out your copyright notice in the Description page of Project Settings.


#include "XPCollectionComponent.h"

#include "XPSubsystem.h"
#include "XPToken.h"
#include "RampageGameMode.h"

#include "Logging/LoggingUtils.h"
#include "TankRampageLogging.h"
#include "VisualLogger/VisualLogger.h"

UXPCollectionComponent::UXPCollectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UXPCollectionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	auto World = GetWorld();
	check(World);

	auto ItemSubsystem = World->GetSubsystem<UXPSubsystem>();
	if (!ensure(ItemSubsystem))
	{
		return;
	}

	ItemSubsystem->OnXPTokenOverlap.AddDynamic(this, &ThisClass::OnTokenCollected);
}

void UXPCollectionComponent::OnTokenCollected(AXPToken* Token, APawn* PlayerPawn)
{
	if (!Token)
	{
		return;
	}

	auto RampageGameMode = Cast<ARampageGameMode>(GetOwner());
	if (!ensure(RampageGameMode))
	{
		return;
	}

	RampageGameMode->OnTokenCollected(*Token);
}

