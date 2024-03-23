// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniNukeProjectile.h"

#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

#include "TRItemLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

void AMiniNukeProjectile::ApplyPostProcessEffects()
{
	auto PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		UE_VLOG_UELOG(this, LogTRItem, Warning, TEXT("%s: ApplyPostProcessEffects: PlayerPawn is NULL"), *GetName());

		return;
	}

	auto CameraComponent = PlayerPawn->FindComponentByClass<UCameraComponent>();
	if (!CameraComponent)
	{
		UE_VLOG_UELOG(this, LogTRItem, Warning, TEXT("%s: ApplyPostProcessEffects: PlayerPawn=%s has NULL CameraComponent"), *GetName(), *PlayerPawn->GetName());

		return;
	}

	auto& CurrentSettings = CameraComponent->PostProcessSettings;
	const auto OriginalSettings = CurrentSettings;

	// TODO: Extract parameters
	CurrentSettings.bOverride_WhiteTemp = true;
	CurrentSettings.WhiteTemp = 100000.0;

	CurrentSettings.bOverride_WhiteTint = true;
	CurrentSettings.WhiteTint = 1.0f;

	CurrentSettings.bOverride_ColorSaturation = true;
	CurrentSettings.ColorSaturation = FVector4{ 2, 0, 2, 2 }; // RGBY

	CurrentSettings.bOverride_VignetteIntensity = true;
	CurrentSettings.VignetteIntensity = 0.9;

	CurrentSettings.bOverride_AutoExposureMinBrightness = CurrentSettings.bOverride_AutoExposureMaxBrightness = true;
	CurrentSettings.AutoExposureMinBrightness = CurrentSettings.AutoExposureMaxBrightness = -4;

	UE_VLOG_UELOG(PlayerPawn, LogTRItem, Log, TEXT("%s: ApplyPostProcessEffects: Begin"), *GetName());

	// Make sure not to capture "this" as object will be destroyed shortly
	// TODO: Interpolate back to original values
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(CameraComponent, [CameraComponent, OriginalSettings, Name = GetName()]()
	{
			CameraComponent->PostProcessSettings = OriginalSettings;
			UE_VLOG_UELOG(CameraComponent->GetOwner(), LogTRItem, Log, TEXT("%s: ApplyPostProcessEffects: End"), *Name);

	}), ShockwaveDuration, false);
}
