// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankCrashComponent.h"

#include "Components/TankTurretComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"

#include "VisualLogger/VisualLogger.h"

#include "Kismet/GameplayStatics.h"

#include "Components/AudioComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TankCrashComponent)

void UTankCrashComponent::RegisterCollisions()
{
	if (!ensureMsgf(TankCrashSfx, TEXT("%s: Has no configured TankCrashSfx"), *GetName()))
	{
		return;
	}

	auto Owner = GetOwner();
	check(Owner);

	// Register for the body and turret
	if (auto TankBody = Cast<UPrimitiveComponent>(Owner->GetRootComponent()); 
		ensureMsgf(TankBody, TEXT("%s: Could not find tank body component (root primitive component")))
	{
		RegisterComponent(TankBody);
	}

	if (auto Turret = Owner->FindComponentByClass<UTankTurretComponent>();
		ensureMsgf(Turret, TEXT("%s: Could not find tank turret component")))
	{
		RegisterComponent(Turret);
	}
}

void UTankCrashComponent::OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse)
{
	check(TankCrashSfx);

	const auto Volume = GetAudioVolume(NormalImpulse);

	UE_VLOG_UELOG(GetOwner(), LogTRTank, Verbose,
		TEXT("%s-%s: OnNotifyRelevantCollision - NormalImpulse=%.3e resulted in Volume=%.2f; Range = [%.1f,%1f]; ImpulseRange=[%.1e,%.1e]"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), NormalImpulse.Size(),
		Volume, MinAudioVolume, MaxAudioVolume,
		MinVolumeImpulse, MaxVolumeImpulse)

	if (FMath::IsNearlyZero(Volume))
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Log,
			TEXT("%s-%s: OnNotifyRelevantCollision - Not playing crash sfx=%s as NormalImpulse=%.3e resulted in 0 Volume"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(TankCrashSfx), NormalImpulse.Size());

		return;
	}

	auto SpawnedAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
		GetOwner(),
		TankCrashSfx,
		Hit.Location, NormalImpulse.Rotation(),
		Volume
	);

	if (!SpawnedAudioComponent)
	{
		// This is not an error condition as the component may not spawn if the sound is not audible, for example it attenuates below a threshold based on distance
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Log,
			TEXT("%s-%s: OnNotifyRelevantCollision - Unable to spawn audio component for sfx=%s; volume=%.2f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(TankCrashSfx), Volume);
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log,
		TEXT("%s-%s: OnNotifyRelevantCollision -  Playing sfx=%s at volume=%.2f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *TankCrashSfx->GetName(), Volume);

	SpawnedAudioComponent->bAutoDestroy = true;
	SpawnedAudioComponent->bReverb = true;
}

float UTankCrashComponent::GetAudioVolume(const FVector& NormalImpulse) const
{
	const double Size = NormalImpulse.Size();

	if (Size < MinVolumeImpulse)
	{
		return 0.0f;
	}
	if (Size >= MaxVolumeImpulse)
	{
		return MaxAudioVolume;
	}

	// Interpolate size between min and max and scale to volume
	const float Alpha = FMath::Min((Size - MinVolumeImpulse) / (static_cast<double>(MaxVolumeImpulse) - MinVolumeImpulse), 1.0);

	return FMath::InterpEaseInOut(MinAudioVolume, MaxAudioVolume, Alpha, VolumeEaseFactor);
}
