// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "XPSubsystem.generated.h"

class AXPToken;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnXPTokenOverlap, AXPToken*, Token, APawn*, PlayerPawn);

/**
 * 
 */
UCLASS()
class TRITEM_API UXPSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnXPTokenOverlap OnXPTokenOverlap;
	
};
