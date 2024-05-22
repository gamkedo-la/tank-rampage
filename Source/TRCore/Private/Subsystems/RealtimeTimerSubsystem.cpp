// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/RealtimeTimerSubsystem.h"

#include "Logging/LoggingUtils.h"
#include "TRCoreLogging.h"

void URealtimeTimerSubsystem::Tick(float DeltaTime)
{
	UE_LOG(LogTRCore, VeryVerbose, TEXT("%s: Tick=%fs"), *GetName(), DeltaTime);

	Super::Tick(DeltaTime);

	if (RealTimeTimerDelegate.IsBound())
	{
		TickDelegates(DeltaTime);
	}
	else
	{
		UE_LOG(LogTRCore, VeryVerbose, TEXT("%s: Tick=%fs - Skipping as no delegates bound"), *GetName(), DeltaTime);
	}
}

TStatId URealtimeTimerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(RealtimeTimerSubsystem, STATGROUP_Tickables);
}

void URealtimeTimerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
}

void URealtimeTimerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URealtimeTimerSubsystem::TickDelegates(float DeltaTime)
{
	UE_LOG(LogTRCore, VeryVerbose, TEXT("%s: TickDelegates=%fs"), *GetName(), DeltaTime);

	RealTimeTimerDelegate.Broadcast(DeltaTime);
}

