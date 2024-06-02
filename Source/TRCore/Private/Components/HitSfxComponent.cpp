// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/HitSfxComponent.h"

#include "Kismet/GameplayStatics.h"

#include "Components/AudioComponent.h"
#include "TRCoreLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(HitSfxComponent)

UHitSfxComponent::UHitSfxComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UHitSfxComponent::ShouldPlaySfx_Implementation(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse) const
{
	if (MaxPlayCount > 0 && PlayCount >= MaxPlayCount)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRCore, Log,
			TEXT("%s-%s: ShouldPlaySfx - Not playing sfx=%s as PlayCount=%d >= MaxPlayCount=%d"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(HitSfx),
			PlayCount, MaxPlayCount);

		return false;
	}

	return true;
}

void UHitSfxComponent::RegisterCollisions()
{
	if (BlueprintRegisterCollisions())
	{
		UE_VLOG_UELOG(GetOwner(), LogTRCore, Warning,
			TEXT("%s-%s: RegisterCollisions - BlueprintRegisterCollisions consumed registration, preventing default behavior"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}

	auto MyOwner = GetOwner();
	if (!ensureMsgf(MyOwner, TEXT("%s: Owner was NULL"), *GetName()))
	{
		return;
	}

	int32 RegisteredComponents{};

	MyOwner->ForEachComponent<UPrimitiveComponent>(false, [&](auto Component)
	{
		++RegisteredComponents;
		UE_VLOG_UELOG(GetOwner(), LogTRCore, Log,
			TEXT("%s-%s: InitEventBindings - Registered hit callback on %s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Component->GetName());

		RegisterComponent(Component);
	});

	if (!RegisteredComponents)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRCore, Warning,
			TEXT("%s-%s: InitEventBindings - Could not find any PrimitiveComponents to register for hit callbacks!"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
	}
}

void UHitSfxComponent::OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse)
{
	check(HitSfx);

	if (!ShouldPlaySfx(HitComponent, Hit, NormalImpulse))
	{
		UE_VLOG_UELOG(GetOwner(), LogTRCore, Log,
			TEXT("%s-%s: OnNotifyRelevantCollision - Not playing sfx=%s as ShouldPlaySfx() returned false"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(HitSfx));
		return;
	}

	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	const auto TimeSeconds = World->GetTimeSeconds();

	if ((MinPlayInterval > 0 && LastPlayTimeSeconds >= 0 && TimeSeconds - LastPlayTimeSeconds < MinPlayInterval) || TimeSeconds < MinPlayTimeSeconds)
	{
		if (TimeSeconds < MinPlayTimeSeconds)
		{
			UE_VLOG_UELOG(GetOwner(), LogTRCore, Log,
				TEXT("%s-%s: OnNotifyRelevantCollision - Not playing sfx=%s as TimeSeconds=%fs < MinPlayTimeSeconds=%fs"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(HitSfx),
				TimeSeconds, MinPlayTimeSeconds);
		}
		else
		{
			UE_VLOG_UELOG(GetOwner(), LogTRCore, Log,
				TEXT("%s-%s: OnNotifyRelevantCollision - Not playing sfx=%s as DeltaTime=%fs < MinPlayInterval=%fs"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(HitSfx),
				TimeSeconds - LastPlayTimeSeconds, MinPlayInterval);
		}

		return;
	}

	const auto Volume = GetAudioVolume(NormalImpulse);

	UE_VLOG_UELOG(GetOwner(), LogTRCore, Verbose,
		TEXT("%s-%s: OnNotifyRelevantCollision - NormalImpulse=%.3e resulted in Volume=%.2f; Range = [%.1f,%1f]; ImpulseRange=[%.1e,%.1e]"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), NormalImpulse.Size(),
		Volume, MinAudioVolume, MaxAudioVolume,
		MinVolumeImpulse, MaxVolumeImpulse);

	if (FMath::IsNearlyZero(Volume, 1e-3))
	{
		UE_VLOG_UELOG(GetOwner(), LogTRCore, Log,
			TEXT("%s-%s: OnNotifyRelevantCollision - Not playing sfx=%s as NormalImpulse=%.3e resulted in 0 Volume"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(HitSfx), NormalImpulse.Size());

		return;
	}

	auto SpawnedAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
		GetOwner(),
		HitSfx,
		Hit.Location, NormalImpulse.Rotation(),
		Volume
	);

	if (!SpawnedAudioComponent)
	{
		// This is not an error condition as the component may not spawn if the sound is not audible,
		// for example it attenuates below a threshold based on distance or is filtered by sound concurrency
		UE_VLOG_UELOG(GetOwner(), LogTRCore, Log,
			TEXT("%s-%s: OnNotifyRelevantCollision - Unable to spawn audio component for sfx=%s; volume=%.3f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(HitSfx), Volume);
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogTRCore, Log,
		TEXT("%s-%s: OnNotifyRelevantCollision -  Playing sfx=%s at volume=%.3f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *HitSfx->GetName(), Volume);

	SpawnedAudioComponent->bAutoDestroy = true;
	SpawnedAudioComponent->bReverb = true;

	LastPlayTimeSeconds = TimeSeconds;

	OnPlaySfx();
}

void UHitSfxComponent::OnPlaySfx_Implementation()
{
	++PlayCount;

	UE_VLOG_UELOG(GetOwner(), LogTRCore, Log,
		TEXT("%s-%s: OnPlaySfx - PlayCount=%d"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(HitSfx),
		PlayCount)
}

float UHitSfxComponent::GetAudioVolume(const FVector& NormalImpulse) const
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
