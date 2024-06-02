// Fill out your copyright notice in the Description page of Project Settings.


#include "BasePlayerController.h"

#include "Logging/LoggingUtils.h"
#include "TRPlayerLogging.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

#include "GameFramework/SpectatorPawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasePlayerController)

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

void ABasePlayerController::SetInputModeUI(UUserWidget* FocusWidget)
{
	// TODO: Detect gamepad input and set to FInputGameModeOnly for that case

	FInputModeUIOnly InputModeData;

	if (FocusWidget)
	{
		InputModeData.SetWidgetToFocus(FocusWidget->TakeWidget());
	}
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputModeData);

	SetShowMouseCursor(true);
}

void ABasePlayerController::SetInputModeGame()
{
	FInputModeGameOnly InputModeData;
	SetInputMode(InputModeData);

	SetShowMouseCursor(false);
}

void ABasePlayerController::RestartLevel()
{
	Super::RestartLevel();
}

void ABasePlayerController::PauseGame(UUserWidget* FocusWidget)
{
	SetInputModeUI(FocusWidget);

	SetPaused(true);
}

void ABasePlayerController::ResumeGame()
{
	SetPaused(false);

	SetInputModeGame();
}

bool ABasePlayerController::IsGamePaused() const
{
	return UGameplayStatics::IsGamePaused(this);
}

void ABasePlayerController::SetPaused(bool bPaused)
{
	UGameplayStatics::SetGamePaused(GetWorld(), bPaused);
}

#pragma region Spectate

void ABasePlayerController::Spectate()
{
	UE_VLOG_UELOG(this, LogTRPlayer, Display, TEXT("%s: Spectate"), *GetName());

	// Allow the spectator pawn to take over the controls; otherwise, some of the bindings will be disabled
	DisableInput(this);

	AddSpectatorPawn();
}

void ABasePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UE_VLOG_UELOG(this, LogTRPlayer, Log, TEXT("%s: OnPossess: %s"), *GetName(), *LoggingUtils::GetName(InPawn));

	LastPossessedPawn = InPawn;
}

void ABasePlayerController::AddSpectatorPawn()
{
	if (!HasAuthority())
	{
		// Can only change state to spectator on the server side
		return;
	}

	UE_VLOG_UELOG(this, LogTRPlayer, Display, TEXT("%s: Changing state to spectator"), *GetName());

	ChangeState(NAME_Spectating);
	ClientGotoState(NAME_Spectating);
	HandleCameraAfterGameEnded();
}

void ABasePlayerController::HandleCameraAfterGameEnded()
{
	if (IsValid(LastPossessedPawn))
	{
		UE_VLOG_UELOG(this, LogTRPlayer, Display, TEXT("%s: HandleCameraAfterGameEnded: Tracking previous player pawn %s"),
			*GetName(),
			*LastPossessedPawn->GetName()
		);

		SetViewTarget(LastPossessedPawn);

		GetWorldTimerManager().SetTimer(SpectatorCameraDelayTimer,
			this, &ThisClass::SetCameraOwnedBySpectatorPawn, SpectatorCameraControlsDelay);

	}
	else
	{
		UE_VLOG_UELOG(this, LogTRPlayer, Warning, TEXT("%s: HandleCameraAfterGameEnded: Previous player pawn NULL - skipping directly to spectator controls"),
			*GetName()
		);

		SetCameraOwnedBySpectatorPawn();
	}
}

void ABasePlayerController::SetCameraOwnedBySpectatorPawn()
{
	auto MySpectatorPawn = GetSpectatorPawn();
	UE_VLOG_UELOG(this, LogTRPlayer, Display, TEXT("%s: Managing camera target with spectator pawn: %s"),
		*GetName(),
		*LoggingUtils::GetName(MySpectatorPawn)
	);

	if (!MySpectatorPawn)
	{
		return;
	}

	if (IsValid(LastPossessedPawn))
	{
		FVector Location(LastPossessedPawn->GetActorLocation());
		Location.Z += SpectatorCameraZOffset;

		// Offset XY opposite to velocity
		FVector DistanceOffsetVector = LastPossessedPawn->GetVelocity();
		DistanceOffsetVector.Z = 0;
		DistanceOffsetVector.Normalize();
		DistanceOffsetVector *= -SpectatorCameraDistanceOffset;

		Location -= DistanceOffsetVector;

		MySpectatorPawn->SetActorLocation(Location);

		const FRotator LookAtRotation = (LastPossessedPawn->GetActorLocation() - Location).Rotation();
		MySpectatorPawn->SetActorRotation(LookAtRotation);
	}

	AutoManageActiveCameraTarget(MySpectatorPawn);
}

#pragma endregion Spectate
