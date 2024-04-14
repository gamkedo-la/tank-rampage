// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ItemSubsystem.generated.h"

class ABasePickup;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUpgraded, UItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLootPickedUp, const ABasePickup*, Pickup, APawn*, PlayerPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLootSpawned, const ABasePickup*, Pickup);

/**
 * 
 */
UCLASS()
class TRITEM_API UItemSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnItemUpgraded OnItemUpgraded;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable, BlueprintCallable)
	FOnLootPickedUp OnLootPickedUp;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnLootSpawned OnLootSpawned;
};
