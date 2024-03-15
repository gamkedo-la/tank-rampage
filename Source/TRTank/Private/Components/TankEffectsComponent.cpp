// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankEffectsComponent.h"

#include "Subsystems/TankEventsSubsystem.h"

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "Pawn/BaseTankPawn.h"

UTankEffectsComponent::UTankEffectsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTankEffectsComponent::BeginPlay()
{
	Super::BeginPlay();

	auto World = GetWorld();
	check(World);

	auto TankEventsSubsystem = World->GetSubsystem<UTankEventsSubsystem>();
	if (ensure(TankEventsSubsystem))
	{
		TankEventsSubsystem->OnTankDestroyed.AddDynamic(this, &ThisClass::OnTankDestroyed);
	}
}

void UTankEffectsComponent::PlayDeathVfx()
{
	if (!DeathVfx)
	{
		UE_VLOG_UELOG(this, LogTRTank, Warning, TEXT("%s: PlayDeathVfx - DeathVfx is not set"), *GetName());
		return;
	}

	const auto& Location = GetOwner()->GetActorLocation();

	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s: PlayHitVfx: %s playing at %s"), *GetName(), *DeathVfx.GetName(), *Location.ToCompactString());

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, DeathVfx, Location);

	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s: PlayHitVfx: %s playing NiagaraComponent=%s"), *GetName(), *DeathVfx.GetName(), *LoggingUtils::GetName(NiagaraComp));

	if (!NiagaraComp)
	{
		return;
	}

	SetNiagaraDeathEffectParameters(NiagaraComp);
}

void UTankEffectsComponent::OnTankDestroyed(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith)
{
	if (DestroyedTank != GetOwner())
	{
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s: OnTankDestroyed"), *GetName());

	PlayDeathVfx();
}

void UTankEffectsComponent::SetNiagaraDeathEffectParameters_Implementation(UNiagaraComponent* NiagaraComponent)
{
	check(NiagaraComponent);

	if (!DeathVfxScaleParameterName.IsNone())
	{
		NiagaraComponent->SetFloatParameter(DeathVfxScaleParameterName, DeathVfxScaleParameterValue);
	}
}
