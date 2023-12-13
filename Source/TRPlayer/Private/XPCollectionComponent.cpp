// Fill out your copyright notice in the Description page of Project Settings.


#include "XPCollectionComponent.h"

#include "XPSubsystem.h"
#include "XPToken.h"

#include "Logging/LoggingUtils.h"
#include "TRPlayerLogging.h"

// Sets default values for his component's properties
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
	if (PlayerPawn != GetOwner())
	{
		UE_LOG(LogTRPlayer, Error, TEXT("%s:%s - OnTokenCollected: PlayerPawn=%s is not owned by this component"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(PlayerPawn));
		return;
	}

	// TODO: Increment level amount and notify UI to update progress bar
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(0, 3.0f, FColor::Green, FString::Printf(TEXT("Collected XP Token %s"), *LoggingUtils::GetName(Token)));
	}
}

