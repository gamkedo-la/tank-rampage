// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/HealthComponent.h"

#include "Subsystems/TankEventsSubsystem.h"

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;

	GetOwner()->OnTakeAnyDamage.AddDynamic(this, &ThisClass::TakeDamage);
}

void UHealthComponent::TakeDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	const auto PreviousHealth = Health;
	Health = FMath::Clamp(Health - Damage, 0, MaxHealth);

	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s-%s: TakeDamage - Amount=%f from %s by %s health changed from %f -> %f"),
		*GetName(), Damage, *LoggingUtils::GetName(DamageCauser), *LoggingUtils::GetName(InstigatedBy), PreviousHealth, Health);

	if (FMath::IsNearlyZero(Health))
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Display, TEXT("%s: TakeDamage - Killed from %s by %s"),
			*GetName(), *LoggingUtils::GetName(DamageCauser), *LoggingUtils::GetName(InstigatedBy));
	}

	if (!FMath::IsNearlyEqual(Health, PreviousHealth))
	{
		OnHealthChanged.Broadcast(this, PreviousHealth, InstigatedBy, DamageCauser);
	}
}