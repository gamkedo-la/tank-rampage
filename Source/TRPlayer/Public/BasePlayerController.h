// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "BasePlayerController.generated.h"

/**
 * 
 */
UCLASS()
class TRPLAYER_API ABasePlayerController : public APlayerController, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()

public:
#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
	void InitDebugDraw();
	void DestroyDebugDraw();
	
private:
	#if ENABLE_VISUAL_LOG
		FTimerHandle VisualLoggerTimer{};
#endif
};
