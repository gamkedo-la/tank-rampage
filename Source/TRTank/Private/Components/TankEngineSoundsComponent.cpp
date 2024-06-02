// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankEngineSoundsComponent.h"

#include "Pawn/BaseTankPawn.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"

#include "VisualLogger/VisualLogger.h"

#include "Kismet/GameplayStatics.h"

#include "Components/AudioComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TankEngineSoundsComponent)

UTankEngineSoundsComponent::UTankEngineSoundsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UTankEngineSoundsComponent::BeginPlay()
{
	Super::BeginPlay();

	InitAudio();
}

void UTankEngineSoundsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CheckEngineAudioComponent())
	{
		return;
	}

	// Sample current state
	const auto AudioParametersOptional = GetEngineAudioParameters();

	if (!AudioParametersOptional)
	{
		UE_VLOG_UELOG(this, LogTRTank, Log,
			TEXT("%s-%s: TickComponent - Cannot compute audio parameters"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		EngineAudioComponent->SetPaused(true);
		return;
	}

	EngineAudioComponent->SetPaused(false);

	auto AudioValues = NextAudioValues(*AudioParametersOptional);
	InterpAudioValues(AudioValues, DeltaTime);
	CurrentEngineAudioValues = AudioValues;

	UpdateAudioComponent();
}

ABaseTankPawn* UTankEngineSoundsComponent::GetOwnerAsTank()
{
	auto OwnerResult = Cast<ABaseTankPawn>(GetOwner());
	ensureMsgf(OwnerResult, TEXT("%s: Component owner %s is not a ABaseTankPawn!"), *GetName(), *LoggingUtils::GetName(GetOwner()));

	return OwnerResult;
}

const ABaseTankPawn* UTankEngineSoundsComponent::GetOwnerAsTank() const
{
	return const_cast<UTankEngineSoundsComponent*>(this)->GetOwnerAsTank();
}

TOptional<UTankEngineSoundsComponent::FEngineAudioParameters> UTankEngineSoundsComponent::GetEngineAudioParameters() const
{
	auto Tank = GetOwnerAsTank();
	if (!Tank)
	{
		return {};
	}

	const auto& ThrottleState = Tank->GetThrottleState();
	
	// Get max value [0,1]
	const auto ThrottleValue = FMath::Max(FMath::Abs(ThrottleState.LeftThrottle), FMath::Abs(ThrottleState.RightThrottle));

	// get the overall tank velocity
	const auto TankVelocity = Tank->GetVelocity();
	const auto TankSpeed = TankVelocity.Size();
	const auto bAirborne = Tank->IsAirborne();

	return FEngineAudioParameters
	{
		.Throttle = ThrottleValue,
		.Speed = static_cast<float>(TankSpeed),
		.bAirborne = bAirborne
	};
}

UAudioComponent* UTankEngineSoundsComponent::CreateEngineAudioComponent() const
{
	if (!ensure(EngineSfx))
	{
		return nullptr;
	}

	auto ComponentOwner = GetOwner();
	if (!ensure(ComponentOwner))
	{
		return nullptr;
	}

	// The owner of the audio component is derived from the world context object and this will control the sound concurrency
	auto SpawnedAudioComponent = UGameplayStatics::SpawnSoundAttached(
		EngineSfx,
		ComponentOwner->GetRootComponent(),
		NAME_None,
		FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset,
		true
	);

	if (!SpawnedAudioComponent)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Warning,
			TEXT("%s-%s: CreateEngineAudioComponent - Unable to spawn audio component for EngineSfx=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *EngineSfx->GetName());
		return nullptr;
	}

	UE_VLOG_UELOG(this, LogTRTank, Log,
		TEXT("%s-%s: CreateEngineAudioComponent - Using EngineSfx=%s with AudioComponent=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *EngineSfx->GetName(), *SpawnedAudioComponent->GetName());

	// Component lifetime managed by this component
	SpawnedAudioComponent->bAutoDestroy = false;
	SpawnedAudioComponent->bReverb = true;
	SpawnedAudioComponent->bAutoActivate = true;

	if (IdleVolumeParameterName.IsNone())
	{
		UE_VLOG_UELOG(this, LogTRTank, Warning,
			TEXT("%s-%s: CreateEngineAudioComponent - IdleVolumeParameterName is NONE for EngineSfx=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *EngineSfx->GetName());
	}

	if (MovementVolumeParameterName.IsNone())
	{
		UE_VLOG_UELOG(this, LogTRTank, Warning,
			TEXT("%s-%s: CreateEngineAudioComponent - MovementVolumeParameterName is NONE for EngineSfx=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *EngineSfx->GetName());
	}

	if (MovementPitchOffsetParameterName.IsNone())
	{
		UE_VLOG_UELOG(this, LogTRTank, Warning,
			TEXT("%s-%s: CreateEngineAudioComponent - MovementPitchOffsetParameterName is NONE for EngineSfx=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *EngineSfx->GetName());
	}

	return SpawnedAudioComponent;
}

UTankEngineSoundsComponent::FEngineAudioValues UTankEngineSoundsComponent::NextAudioValues(const FEngineAudioParameters& Parameters) const
{
	if (Parameters.bAirborne)
	{
		return DefaultAudioValues();
	}

	const auto SpeedFraction = Parameters.Speed / MovementAudioMaxSpeed;

	FEngineAudioValues NewAudioValues;

	NewAudioValues.MovementVolume = FMath::Lerp(MinMovementVolume, MaxMovementVolume, SpeedFraction);
	NewAudioValues.MovementPitchShift = FMath::Lerp(MinMovementPitchShift, MaxMovementPitchShift, SpeedFraction);
	NewAudioValues.IdleVolume = FMath::Lerp(MinIdleVolume, MaxIdleVolume, 1 - SpeedFraction);

	return NewAudioValues;
}

UTankEngineSoundsComponent::FEngineAudioValues UTankEngineSoundsComponent::DefaultAudioValues() const
{
	return FEngineAudioValues
	{
		.IdleVolume = MaxIdleVolume,
		.MovementVolume = MinMovementVolume,
		.MovementPitchShift = MinMovementPitchShift
	};
}

void UTankEngineSoundsComponent::InterpAudioValues(FEngineAudioValues& NewAudioValues, float DeltaTime) const
{
	NewAudioValues.IdleVolume = InterpValue(CurrentEngineAudioValues.IdleVolume, NewAudioValues.IdleVolume, DeltaTime);
	NewAudioValues.MovementVolume = InterpValue(CurrentEngineAudioValues.MovementVolume, NewAudioValues.MovementVolume, DeltaTime);
	NewAudioValues.MovementPitchShift = InterpValue(CurrentEngineAudioValues.MovementPitchShift, NewAudioValues.MovementPitchShift, DeltaTime);
}

bool UTankEngineSoundsComponent::CheckEngineAudioComponent()
{
	if (!EngineAudioComponent)
	{
		// disable tick since the component
		PrimaryComponentTick.SetTickFunctionEnable(false);
		return false;
	}

	return true;
}

void UTankEngineSoundsComponent::InitAudio()
{
	EngineAudioComponent = CreateEngineAudioComponent();
	if (!CheckEngineAudioComponent())
	{
		return;
	}

	// Set initial parameter values
	CurrentEngineAudioValues = DefaultAudioValues();
}

void UTankEngineSoundsComponent::UpdateAudioComponent()
{
	check(EngineAudioComponent);

	UE_VLOG_UELOG(this, LogTRTank, VeryVerbose,
		TEXT("%s-%s: UpdateAudioComponent - UpdateAudioComponent=%s; IdleVolume=%.2f; MovementVolume=%.2f; MovementPitchShift=%.2f"),
		*GetName(), *LoggingUtils::GetName(GetOwner()),
		*EngineAudioComponent->GetName(), CurrentEngineAudioValues.IdleVolume, CurrentEngineAudioValues.MovementVolume, CurrentEngineAudioValues.MovementPitchShift
	);

	if (!IdleVolumeParameterName.IsNone())
	{
		EngineAudioComponent->SetFloatParameter(IdleVolumeParameterName, CurrentEngineAudioValues.IdleVolume);
	}

	if (!MovementVolumeParameterName.IsNone())
	{
		EngineAudioComponent->SetFloatParameter(MovementVolumeParameterName, CurrentEngineAudioValues.MovementVolume);
	}

	if (!MovementPitchOffsetParameterName.IsNone())
	{
		EngineAudioComponent->SetFloatParameter(MovementPitchOffsetParameterName, CurrentEngineAudioValues.MovementPitchShift);
	}
}

float UTankEngineSoundsComponent::InterpValue(float Previous, float Current, float DeltaTime) const
{
	return FMath::FInterpTo(Previous, Current, DeltaTime, AudioInterpTime);
}
