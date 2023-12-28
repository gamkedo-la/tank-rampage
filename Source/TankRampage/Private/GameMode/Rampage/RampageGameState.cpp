// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/RampageGameState.h"

#include "LevelUnlocksComponent.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "TankRampageLogging.h"

#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"

void ARampageGameState::ApplyLevelUnlock(APawn* Pawn, const FLevelUnlock& Unlock)
{
	auto GameMode = UGameplayStatics::GetGameMode(this);
	if (!GameMode)
	{
		// TODO: In multiplayer need to check for authority and dispatch to the server version that has access to the game mode
		UE_VLOG_UELOG(this, LogTankRampage, Error, TEXT("%s: ApplyLevelUnlock - GameMode is not available; Pawn=%s; Unlock=%s"),
			*GetName(), *LoggingUtils::GetName(Pawn), *Unlock.Description);

		return;
	}

	auto LevelUnlocksComponent = GameMode->FindComponentByClass<ULevelUnlocksComponent>();

	if (!LevelUnlocksComponent)
	{
		UE_VLOG_UELOG(this, LogTankRampage, Error, TEXT("%s: ApplyLevelUnlock - GameMode %s does not have a LevelUnlocksComponent; Pawn=%s; Unlock=%s"),
			*GetName(), *LoggingUtils::GetName(GameMode), *LoggingUtils::GetName(Pawn), *Unlock.Description);

		return;
	}

	LevelUnlocksComponent->ApplyLevelUnlock(Pawn, Unlock);
}
