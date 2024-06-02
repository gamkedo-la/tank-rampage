// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/HitSfxComponent.h"
#include "TankCrashComponent.generated.h"

class USoundBase;

/**
 * 
 */
UCLASS()
class UTankCrashComponent : public UHitSfxComponent
{
	GENERATED_BODY()

protected:
	virtual void RegisterCollisions() override;
};
