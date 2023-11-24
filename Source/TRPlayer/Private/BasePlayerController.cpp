// Fill out your copyright notice in the Description page of Project Settings.


#include "BasePlayerController.h"

#include "Logging/LoggingUtils.h"
#include "TRPlayerLogging.h"

void ABasePlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitDebugDraw();
}

void ABasePlayerController::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	DestroyDebugDraw();
}

void ABasePlayerController::InitDebugDraw()
{
// Ensure that state logged regularly so we see the updates in the visual logger
#if ENABLE_VISUAL_LOG

	FTimerDelegate DebugDrawDelegate = FTimerDelegate::CreateLambda([this]()
	{
			UE_VLOG(this, LogTRPlayer, Log, TEXT("Get Tank State"));
	});

	GetWorldTimerManager().SetTimer(VisualLoggerTimer, DebugDrawDelegate, 0.05f, true);

#endif
}

void ABasePlayerController::DestroyDebugDraw()
{
#if ENABLE_VISUAL_LOG

	GetWorldTimerManager().ClearTimer(VisualLoggerTimer);

#endif
}

#if ENABLE_VISUAL_LOG
void ABasePlayerController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{

}

#endif
