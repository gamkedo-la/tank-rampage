// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniNukeProjectile.h"

#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

#include "Item/WeaponConfig.h"

#include "TRItemLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

void AMiniNukeProjectile::ApplyPostHitEffects(const FHitResult& HitInfo, const FProjectileDamageParams& DamageParams)
{
	ApplyPostProcessEffects();
	ApplyCameraShake(HitInfo, DamageParams);
}

void AMiniNukeProjectile::ApplyPostProcessEffects()
{
	if (!bEnablePostProcess)
	{
		UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: ApplyPostProcessEffects: Disabled"), *GetName());
		return;
	}

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

	const auto OriginalSettings = CameraComponent->PostProcessSettings;
	CameraComponent->PostProcessSettings = PostProcessSettings;

	auto World = GetWorld();
	check(World);

	const auto CurrentTimeSeconds = World->GetTimeSeconds();

	if (!ensureMsgf(PostProcessMinDuration <= PostProcessMaxDuration, TEXT("PostProcessMinDuration=%f > PostProcessMaxDuration=%f"), PostProcessMinDuration, PostProcessMaxDuration))
	{
		PostProcessMaxDuration = PostProcessMinDuration;
	}

	const auto PostProcessFadeStartTimeSeconds = CurrentTimeSeconds + PostProcessMinDuration;
	const auto PostProcessFadeEndTimeSeconds = CurrentTimeSeconds + PostProcessMaxDuration;

	UE_VLOG_UELOG(PlayerPawn, LogTRItem, Log, TEXT("%s: ApplyPostProcessEffects: Begin - TotalDuration=%fs; FadeDuration=%fs"), *GetName(), PostProcessMaxDuration, PostProcessMaxDuration - PostProcessMinDuration);

	// Make sure not to capture "this" as object will be destroyed shortly
	FTimerHandle InterpolationTimerHandle;

	if (PostProcessFadeEndTimeSeconds > PostProcessFadeStartTimeSeconds)
	{
		GetWorldTimerManager().SetTimer(InterpolationTimerHandle, FTimerDelegate::CreateWeakLambda(CameraComponent,
			[World, CameraComponent, OriginalSettings, NewSettings = PostProcessSettings, FadeDuration = PostProcessFadeEndTimeSeconds - PostProcessFadeStartTimeSeconds, PostProcessFadeStartTimeSeconds, Name = GetName()]()
			{
				const auto Alpha = FMath::Clamp((World->GetTimeSeconds() - PostProcessFadeStartTimeSeconds) / FadeDuration, 0.0f, 1.0f);
				auto& CurrentSettings = CameraComponent->PostProcessSettings;

				if (CurrentSettings.bOverride_WhiteTemp)
				{
					CurrentSettings.WhiteTemp = FMath::Lerp(NewSettings.WhiteTemp, OriginalSettings.WhiteTemp, Alpha);
				}
				if (CurrentSettings.bOverride_WhiteTint)
				{
					CurrentSettings.WhiteTint = FMath::Lerp(NewSettings.WhiteTint, OriginalSettings.WhiteTint, Alpha);
				}
				if(CurrentSettings.bOverride_ColorSaturation)
				{
					CurrentSettings.ColorSaturation = FMath::Lerp(NewSettings.ColorSaturation, OriginalSettings.ColorSaturation, Alpha);
				}
				if (CurrentSettings.bOverride_SceneFringeIntensity)
				{
					CurrentSettings.SceneFringeIntensity = FMath::Lerp(NewSettings.SceneFringeIntensity, OriginalSettings.SceneFringeIntensity, Alpha);
				}
				if (CurrentSettings.bOverride_BloomIntensity)
				{
					CurrentSettings.BloomIntensity = FMath::Lerp(NewSettings.BloomIntensity, OriginalSettings.BloomIntensity, Alpha);
				}
				if (CurrentSettings.bOverride_FilmGrainIntensity)
				{
					CurrentSettings.FilmGrainIntensity = FMath::Lerp(NewSettings.FilmGrainIntensity, OriginalSettings.FilmGrainIntensity, Alpha);
				}

				UE_VLOG_UELOG(CameraComponent->GetOwner(), LogTRItem, VeryVerbose, TEXT("%s: ApplyPostProcessEffects: Fade out - %.1f %% "), *Name, Alpha * 100);

			}), PostProcessInterpolateInterval, true, PostProcessMinDuration);
	} // Fading enabled

	// Clear the effect at MaxDuration
	FTimerHandle EndTimerHandle;
	GetWorldTimerManager().SetTimer(EndTimerHandle, FTimerDelegate::CreateWeakLambda(CameraComponent,
		[World, InterpolationTimerHandle, CameraComponent, OriginalSettings, Name = GetName()]() mutable
	{
		World->GetTimerManager().ClearTimer(InterpolationTimerHandle);
		CameraComponent->PostProcessSettings = OriginalSettings;

		UE_VLOG_UELOG(CameraComponent->GetOwner(), LogTRItem, Log, TEXT("%s: ApplyPostProcessEffects: End"), *Name);

	}), PostProcessMaxDuration, false);
}

void AMiniNukeProjectile::ApplyCameraShake(const FHitResult& HitInfo, const FProjectileDamageParams& DamageParams) const
{
	if (!ensureMsgf(ExplosionCameraShake, TEXT("%s: ExplosionCameraShake not set"), *GetName()))
	{
		return;
	}

	UGameplayStatics::PlayWorldCameraShake(this, ExplosionCameraShake, 
		HitInfo.Location,
		DamageParams.DamageInnerRadius * CameraShakeInnerRadiusMultiplier,
		DamageParams.DamageOuterRadius * CameraShakeOuterRadiusMultiplier,
		true);
}
