// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ItemSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUpgraded, UItem*, Item);

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
};
