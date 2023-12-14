// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XPCollectionComponent.generated.h"

class AXPToken;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UXPCollectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UXPCollectionComponent();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnTokenCollected(AXPToken* Token, APawn* PlayerPawn);
};
