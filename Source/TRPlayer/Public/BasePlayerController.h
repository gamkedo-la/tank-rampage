// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "BasePlayerController.generated.h"

class UUserWidget;

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

	UFUNCTION(BlueprintCallable)
	void SetInputModeUI(UUserWidget* FocusWidget = nullptr);

	UFUNCTION(BlueprintCallable)
	void SetInputModeGame();

	virtual void RestartLevel() override;

	UFUNCTION(BlueprintCallable)
	virtual void PauseGame(UUserWidget* FocusWidget = nullptr);

	UFUNCTION(BlueprintCallable)
	virtual void ResumeGame();

	UFUNCTION(BlueprintPure)
	bool IsGamePaused() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
	void InitDebugDraw();
	void DestroyDebugDraw();
	void SetPaused(bool bPaused);
	
private:
	#if ENABLE_VISUAL_LOG
		FTimerHandle VisualLoggerTimer{};
#endif
};
