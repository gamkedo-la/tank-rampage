// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/LevelUnlocksComponent.h"

#include "Pawn/BaseTankPawn.h"
#include "Item/ItemInventory.h"
#include "Item/Item.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "TankRampageLogging.h"
#include "Utils/RandUtils.h"

#include "Kismet/GameplayStatics.h"

#include <array>
#include <numeric>

namespace
{
	constexpr int32 MaxOptions = 64;
	using IndexArray = std::array<int32, MaxOptions>;

	constexpr int32 GetNumUnlockOptions(const FLevelUnlocksConfig& Config)
	{
		// Don't allow more than IndexArray number of options
		return FMath::Min(MaxOptions, Config.MaxUnlockOptions);
	}

	template<typename Random>
	void SelectRandomizedAvailableItems(Random& Rng, const TArray<FLevelUnlock>& PossibleUnlocks, TArray<FLevelUnlock>& OutAvailable, int32 Count);
}

// Initialize the random number generator with a seed from current time
ULevelUnlocksComponent::ULevelUnlocksComponent() : Rng(RandUtils::GenerateSeed())
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULevelUnlocksComponent::BeginPlay()
{
	Super::BeginPlay();

	GiveLocalPlayerFirstLevelUnlocks();
}

void ULevelUnlocksComponent::GiveLocalPlayerFirstLevelUnlocks() const
{
	if (const auto& FirstLevelUnlockOpt = GetFirstLevelUnlockOptions(); FirstLevelUnlockOpt && FirstLevelUnlockOpt->Config)
	{
		auto World = GetWorld();
		check(World);

		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, FirstLevelUnlockOpt]
		{
			APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
			for (const auto& LevelUnlock : FirstLevelUnlockOpt->Config.AvailableUnlocks)
			{
				this->ApplyLevelUnlock(PlayerPawn, LevelUnlock);
			}
		}));
	}
}

std::optional<FLevelUnlocksContext> ULevelUnlocksComponent::GetNextLevelUnlockOptions(APawn* Pawn, int32 NextLevel) const
{
	checkf(NextLevel >= 0, TEXT("NextLevel=%d < 0"), NextLevel);

	// Make sure at least one level unlock is available
	if (!ensure(!LevelUnlocks.IsEmpty()))
	{
		return std::nullopt;
	}

	const auto& ItemInventory = GetItemInventory(Pawn);
	if (!ItemInventory)
	{
		UE_VLOG_UELOG(this, LogTankRampage, Error, TEXT("%s: GetNextLevelUnlockOptions: No unlocks available as Pawn=%s did not have an inventory"),
			*GetName(), *LoggingUtils::GetName(Pawn));

		return std::nullopt;
	}

	int32 NumCurrent, NumAvailableOptions;
	DetermineAvailableOptionCounts(NextLevel, NumCurrent, NumAvailableOptions);

	const TArray<FLevelUnlock> AvailableUnlocks = GetAvailableUnlocks(NextLevel, ItemInventory, NumCurrent, NumAvailableOptions);

	if (AvailableUnlocks.Num() != NumAvailableOptions)
	{
		UE_VLOG_UELOG(this, LogTankRampage, Warning,
			TEXT("%s: GetNextLevelUnlockOptions: Unable to populate all item choices - Pawn=%s; NextLevel=%d; Count=%d; NumCurrent=%d; NumAvailableOptions=%d; NumPrevious=%d"),
			*GetName(), *LoggingUtils::GetName(Pawn), NextLevel, AvailableUnlocks.Num(), NumCurrent, NumAvailableOptions, NumAvailableOptions - NumCurrent);
	}

	NumAvailableOptions = FMath::Min(NumAvailableOptions, AvailableUnlocks.Num());

	if (NumAvailableOptions == 0)
	{
		UE_VLOG_UELOG(this, LogTankRampage, Warning, TEXT("%s: GetNextLevelUnlockOptions - Pawn=%s; NextLevel=%d; No more unlocks available!"),
			*GetName(), *LoggingUtils::GetName(Pawn), NextLevel);

		return std::nullopt;
	}

	return GetLevelUnlocksContext(NextLevel, AvailableUnlocks, NumAvailableOptions);
}

void ULevelUnlocksComponent::DetermineAvailableOptionCounts(int32 NextLevel, int32& NumCurrent, int32& NumAvailableOptions) const
{
	// Offer 0 to N-1 previous upgrades passed on previously - if at the end of level - keep offering available previous upgrades

	if (NextLevel >= LevelUnlocks.Num())
	{
		UE_VLOG_UELOG(this, LogTankRampage, Display, TEXT("%s: DetermineAvailableOptionCounts : NextLevel=%d >= LevelUnlocks.Num()=%d - Offering previous unlocks"),
			*GetName(), NextLevel, LevelUnlocks.Num());

		NumAvailableOptions = GetNumUnlockOptions(LevelUnlocks.Last());
		NumCurrent = 0;
	}
	else
	{
		const auto& Config = LevelUnlocks[NextLevel];
		NumAvailableOptions = GetNumUnlockOptions(Config);
		NumCurrent = NumAvailableOptions > 0 ? FMath::Min(FMath::RandRange(1, NumAvailableOptions), Config.AvailableUnlocks.Num()) : 0;
	}
}

TArray<FLevelUnlock> ULevelUnlocksComponent::GetAvailableUnlocks(const int32 NextLevel, const UItemInventory* ItemInventory, int32& NumCurrent, const int32 NumAvailableOptions) const
{
	const auto& CurrentItems = ItemInventory->GetCurrentItems();

	// Current selections wouldn't be available if none were configured for that level or we are already at the max levels
	const bool bCurrentIsAvailable = NumCurrent > 0;

	TArray<FLevelUnlock> PossibleCurrentUnlocks;

	if (bCurrentIsAvailable)
	{
		PossibleCurrentUnlocks.Reserve(MaxOptions);

		PopulateViableUnlockOptions(CurrentItems, LevelUnlocks[NextLevel].AvailableUnlocks, PossibleCurrentUnlocks);

		// Not all current unlocks will be viable if previous levels not earned
		NumCurrent = FMath::Min(NumCurrent, PossibleCurrentUnlocks.Num());
	}

	TArray<FLevelUnlock> PossiblePreviousUnlocks;

	if (NumCurrent < NumAvailableOptions)
	{
		PossiblePreviousUnlocks.Reserve(MaxOptions);
		// Start at previous level and iterate backwards
		for (int32 i = FMath::Min(NextLevel, LevelUnlocks.Num()) - 1; i >= 0; --i)
		{
			PopulateViableUnlockOptions(CurrentItems, LevelUnlocks[i].AvailableUnlocks, PossiblePreviousUnlocks);
		}
	}

	int32 NumPrevious = FMath::Min(NumAvailableOptions - NumCurrent, PossiblePreviousUnlocks.Num());

	// If there is a shortfall - apply more currents if possible
	if (bCurrentIsAvailable && NumCurrent + NumPrevious < NumAvailableOptions)
	{
		NumCurrent = FMath::Min(PossibleCurrentUnlocks.Num(), NumAvailableOptions - NumPrevious);
	}
	// Take rest from previous
	if (NumCurrent + NumPrevious < NumAvailableOptions)
	{
		NumPrevious = FMath::Min(NumAvailableOptions - NumCurrent, PossiblePreviousUnlocks.Num());
	}

	TArray<FLevelUnlock> AvailableUnlocks;
	AvailableUnlocks.Reserve(NumAvailableOptions);

	if (NumCurrent > 0)
	{
		SelectRandomizedAvailableItems(Rng, PossibleCurrentUnlocks, AvailableUnlocks, NumCurrent);
	}
	if (NumPrevious > 0)
	{
		SelectRandomizedAvailableItems(Rng, PossiblePreviousUnlocks, AvailableUnlocks, NumPrevious);
	}

	return AvailableUnlocks;
}

void ULevelUnlocksComponent::ApplyLevelUnlock(APawn* Pawn, const FLevelUnlock& Unlock) const
{
	UE_VLOG_UELOG(this, LogTankRampage, Display, TEXT("%s: ApplyLevelUnlock - Pawn=%s; Unlock=%s"),
		*GetName(), *LoggingUtils::GetName(Pawn), *Unlock.Description.ToString());

	auto Tank = Cast<ABaseTankPawn>(Pawn);

	if (!Tank)
	{
		UE_VLOG_UELOG(this, LogTankRampage, Error, TEXT("%s: ApplyLevelUnlock - Pawn=%s is not a Tank! Unlock=%s"),
			*GetName(), *LoggingUtils::GetName(Pawn), *Unlock.Description.ToString());

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
		if (ensureAlwaysMsgf(Item, TEXT("ItemName=%s exists in inventory"), *Unlock.ItemName.ToString()))
		{
			Item->SetLevel(Unlock.Level);
		}
	}
}

UItemInventory* ULevelUnlocksComponent::GetItemInventory(APawn* Pawn) const
{
	auto Tank = Cast<ABaseTankPawn>(Pawn);

	if (!Tank)
	{
		UE_VLOG_UELOG(this, LogTankRampage, Error, TEXT("%s: GetItemInventory - Pawn=%s is not a Tank!"),
			*GetName(), *LoggingUtils::GetName(Pawn));

		return nullptr;
	}

	return Tank->GetItemInventory();
}

std::optional<FLevelUnlocksContext> ULevelUnlocksComponent::GetFirstLevelUnlockOptions() const
{
	// Make sure at least one level unlock is available
	if (!ensure(!LevelUnlocks.IsEmpty()))
	{
		return std::nullopt;
	}

	return GetLevelUnlocksContext(1, LevelUnlocks[0].AvailableUnlocks, LevelUnlocks[0].AvailableUnlocks.Num());
}

FLevelUnlocksContext ULevelUnlocksComponent::GetLevelUnlocksContext(int32 NextLevel, const TArray<FLevelUnlock>& TotalOptions, int32 NumAvailableOptions) const
{
	return FLevelUnlocksContext
	{
		.Config = FLevelUnlocksConfig
		{
			.AvailableUnlocks = TotalOptions,
			.MaxUnlockOptions = NumAvailableOptions
		},
		.Level = NextLevel
	};
}

void ULevelUnlocksComponent::PopulateViableUnlockOptions(const TArray<UItem*>& CurrentItems, const TArray<FLevelUnlock>& TotalOptions, TArray<FLevelUnlock>& OutOptions) const
{
	for (const auto& AvailableOption : TotalOptions)
	{
		auto PossibleItemMatch = CurrentItems.FindByPredicate([&](auto Item) {
			return AvailableOption.ItemName == Item->GetName();
		});

		if ((AvailableOption.Level == 1 && !PossibleItemMatch) ||
			(PossibleItemMatch && AvailableOption.Level == (*PossibleItemMatch)->GetLevel() + 1))
		{
			OutOptions.Add(AvailableOption);
		}
	}
}

namespace
{
	template<typename Random>
	void SelectRandomizedAvailableItems(Random& Rng, const TArray<FLevelUnlock>& PossibleUnlocks, TArray<FLevelUnlock>& OutAvailable, int32 Count)
	{
		Count = FMath::Min(Count, PossibleUnlocks.Num());

		// Should be uninitialized as we initialize it in ShuffleIndices
		IndexArray Indices;
		RandUtils::ShuffleIndices(Indices.begin(), Rng, PossibleUnlocks.Num());

		for (int32 i = 0; i < Count; ++i)
		{
			OutAvailable.Add(PossibleUnlocks[Indices[i]]);
		}
	}
}