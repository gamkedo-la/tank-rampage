// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/RampageGameMode.h"

#include "XPSpawnerComponent.h"
#include "XPCollectionComponent.h"
#include "RampageGameState.h"
#include "Pawn/BaseTankPawn.h"
#include "Components/HealthComponent.h"
#include "Subsystems/TankEventsSubsystem.h"
#include "XPToken.h"
#include "XPSubsystem.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "TankRampageLogging.h"
#include "GameFramework/PlayerController.h"

#include <limits>

ARampageGameMode::ARampageGameMode()
{
	XPSpawnerComponent = CreateDefaultSubobject<UXPSpawnerComponent>(TEXT("XP Spawner"));
	XPCollectionComponent = CreateDefaultSubobject<UXPCollectionComponent>(TEXT("XP Collection"));
}

void ARampageGameMode::OnTokenCollected(const AXPToken& Token, APawn* PlayerPawn)
{
	UE_VLOG_UELOG(this, LogTankRampage, Log, TEXT("%s: OnTokenCollected: Token=%s by PlayerPawn=%s"),
		*GetName(), *Token.GetName(), *LoggingUtils::GetName(PlayerPawn));

	if (auto Tank = Cast<ABaseTankPawn>(PlayerPawn); !Tank || Tank->GetHealthComponent()->IsDead())
	{
		UE_VLOG_UELOG(this, LogTankRampage, Log, TEXT("%s: OnTokenCollected: Skipping token collection as PlayerPawn=%s is DEAD"),
			*GetName(), *LoggingUtils::GetName(PlayerPawn));

		return;
	}

	AddXP(TokenXPAmount);
}

void ARampageGameMode::BeginPlay()
{
	Super::BeginPlay();

	InitializeGameState();
	RegisterEvents();
}

void ARampageGameMode::AddXP(int32 XP)
{
	auto RampageGameState = GetGameState<ARampageGameState>();
	if (!ensure(RampageGameState))
	{
		return;
	}

	const auto AppliedXP = FMath::Min(XP, RampageGameState->LevelUpXP - XP);
	RampageGameState->CurrentXP += AppliedXP;
	RampageGameState->TotalXP += AppliedXP;

	UE_VLOG_UELOG(this, LogTankRampage, Log, TEXT("%s: AddXP: XP=%d; AppliedXP=%d; Level=%d; XP Progress = %d / %d = %f; TotalXP=%d"),
		*GetName(), XP, AppliedXP, RampageGameState->Level, RampageGameState->CurrentXP, RampageGameState->LevelUpXP, RampageGameState->TotalXP);

	if (RampageGameState->CurrentXP >= RampageGameState->LevelUpXP)
	{
		++RampageGameState->Level;

		int32 NextLevelXP;

		if (RampageGameState->Level < XPLevels.Num())
		{
			NextLevelXP = XPLevels[RampageGameState->Level];
		}
		else
		{
			NextLevelXP = !XPLevels.IsEmpty() ? XPLevels.Last() : std::numeric_limits<int32>::max();

			UE_VLOG_UELOG(this, LogTankRampage, Warning, TEXT("%s: New Level=%d over max configured XP levels=%d - clamping next level xp to last=%d"),
				*GetName(), RampageGameState->Level + 1, XPLevels.Num(), NextLevelXP);
		}

		RampageGameState->CurrentXP = 0;
		RampageGameState->LevelUpXP = NextLevelXP;

		UE_LOG(LogTankRampage, Display, TEXT("%s: Level up to %d"), *GetName(), RampageGameState->Level + 1);

		auto World = GetWorld();
		check(World);

		if (auto XPSubsystem = World->GetSubsystem<UXPSubsystem>(); ensure(XPSubsystem))
		{
			XPSubsystem->OnXPLevelUp.Broadcast();
		}
	}
}

void ARampageGameMode::InitializeGameState()
{
	auto RampageGameState = GetGameState<ARampageGameState>();
	if (!ensure(RampageGameState))
	{
		return;
	}

	if (XPLevels.IsEmpty())
	{
		UE_LOG(LogTankRampage, Error, TEXT("%s: No XP Levels set - No progression will happen!"), *GetName());
		RampageGameState->LevelUpXP = std::numeric_limits<int32>::max();
	}
	else
	{
		RampageGameState->LevelUpXP = XPLevels[0];
	}
}

void ARampageGameMode::RegisterEvents()
{
	auto World = GetWorld();
	check(World);

	auto TankEventsSubsystem = World->GetSubsystem<UTankEventsSubsystem>();
	if (ensure(TankEventsSubsystem))
	{
		TankEventsSubsystem->OnTankDestroyed.AddDynamic(this, &ThisClass::OnTankDestroyed);
	}
}

void ARampageGameMode::OnTankDestroyed(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith)
{
	check(DestroyedTank);

	// TODO: We may want to consider spawning additional AI tanks as they are killed

	auto Controller = DestroyedTank->GetController();
	if (auto PlayerController = Cast<APlayerController>(Controller); PlayerController)
	{
		UE_VLOG_UELOG(this, LogTankRampage, Display,
			TEXT("%s: Player %s-%s killed"), *GetName(), *DestroyedTank->GetName(), *Controller->GetName());
		
		PlayerController->GameHasEnded(DestroyedTank, false);
	}
}
