// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/GameStatisticsSubsystem.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "TankRampageLogging.h"

#include "Subsystems/TankEventsSubsystem.h"
#include "Pawn/BaseTankPawn.h"

#include "Components/HealthComponent.h"

#include "Kismet/GameplayStatics.h"

void UGameStatisticsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Register delegates
	if (auto TankEventsSubsystem = InWorld.GetSubsystem<UTankEventsSubsystem>();
		ensureMsgf(TankEventsSubsystem, TEXT("%s: TankEventsSubsystem is NULL"), *GetName()))
	{
		TankEventsSubsystem->OnTankDestroyed.AddDynamic(this, &ThisClass::OnTankDestroyed);
	}
}

void UGameStatisticsSubsystem::OnTankDestroyed(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith)
{
	check(DestroyedTank);

	if (!DestroyedBy || !DestroyedBy->IsPlayerController())
	{
		return;
	}

	if (!DestroyedTank || DestroyedTank->GetController() == DestroyedBy)
	{
		return;
	}

	if (!ShouldRecordUpdate())
	{
		return;
	}

	++EnemiesKilled;

	UE_VLOG_UELOG(DestroyedBy, LogTankRampage, Log, TEXT("%s: OnTankDestroyed: %s with %s; EnemiesKilled=%d"),
		*GetName(),
		*LoggingUtils::GetName(DestroyedTank),
		*LoggingUtils::GetName(DestroyedWith),
		EnemiesKilled
	);

}

bool UGameStatisticsSubsystem::ShouldRecordUpdate() const
{
	auto PlayerTank = Cast<ABaseTankPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

	return PlayerTank && PlayerTank->GetHealthComponent()->IsAlive();
}
