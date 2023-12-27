// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/LevelUnlocksComponent.h"

#include "Pawn/BaseTankPawn.h"
#include "Item/ItemInventory.h"
#include "Item/Item.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "TankRampageLogging.h"

#include "Kismet/GameplayStatics.h"

ULevelUnlocksComponent::ULevelUnlocksComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

std::optional<FLevelUnlocksContext> ULevelUnlocksComponent::GetNextLevelUnlockOptions(int32 NextLevel) const
{
	// TODO: The available upgrades needs to consider what's already in the inventory and what's available at that level (and some randomness) with only a preset number of available options
	if (NextLevel >= LevelUnlocks.Num())
	{
		UE_VLOG_UELOG(this, LogTankRampage, Warning, TEXT("%s: GetNextLevelUnlockOptions : NextLevel=%d >= LevelUnlocks.Num()=%d - no more unlocks available!"),
			*GetName(), NextLevel, LevelUnlocks.Num());

		return std::nullopt;
	}

	return FLevelUnlocksContext
	{
		.Config = LevelUnlocks[NextLevel],
		.Level = NextLevel
	};
}

void ULevelUnlocksComponent::ApplyLevelUnlock(APawn* Pawn, const FLevelUnlock& Unlock)
{
	UE_VLOG_UELOG(this, LogTankRampage, Display, TEXT("%s: ApplyLevelUnlock : Pawn=%s; Unlock=%s"),
		*GetName(), *LoggingUtils::GetName(Pawn), *Unlock.Description);

	auto Tank = Cast<ABaseTankPawn>(Pawn);

	if (!Tank)
	{
		UE_VLOG_UELOG(this, LogTankRampage, Error, TEXT("%s: ApplyLevelUnlock: Pawn=%s is not a Tank! Unlock=%s"),
			*GetName(), *LoggingUtils::GetName(Pawn), *Unlock.Description);

		return;
	}

	auto Inventory = Tank->GetItemInventory();

	if (Unlock.Level == 1)
	{
		Inventory->AddItemByName(Unlock.ItemName);
	}
	else
	{
		auto Item = Inventory->GetItemByName(Unlock.ItemName);
		if (ensureAlwaysMsgf(Item, TEXT("ItemName=%s exists in inventory")), *Unlock.ItemName.ToString())
		{
			Item->SetLevel(Unlock.Level);
		}
	}
}

void ULevelUnlocksComponent::BeginPlay()
{
	Super::BeginPlay();

	// apply first unlock automatically
	if (const auto& FirstLevelUnlockOpt = GetNextLevelUnlockOptions(1); FirstLevelUnlockOpt && FirstLevelUnlockOpt->Config)
	{
		auto World = GetWorld();
		check(World);

		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, FirstLevelUnlockOpt]
		{
			this->ApplyLevelUnlock(UGameplayStatics::GetPlayerPawn(this, 0), FirstLevelUnlockOpt->Config.AvailableUnlocks[0]);
		}));
	}
}
