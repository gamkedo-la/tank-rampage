// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RealtimeTimerSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FRealTimeTimerDelegate, float /* DeltaTime */)

/**
 * Allows ticking delegate functions when game is paused, but all ticks are done in realtime rather than game time.
 * Useful for UI ticks that do not happen from a <c>UUserWidget</c> where ticking when paused could be explicitly configured.
 * FTimerManager does not support ticking when paused
 */
UCLASS()
class TRCORE_API URealtimeTimerSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	FRealTimeTimerDelegate RealTimeTimerDelegate{};

protected:
	virtual bool IsTickableWhenPaused() const override { return true;  }

	virtual void Tick(float DeltaTime) override;

	virtual TStatId GetStatId() const override;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void TickDelegates(float DeltaTime);
};
