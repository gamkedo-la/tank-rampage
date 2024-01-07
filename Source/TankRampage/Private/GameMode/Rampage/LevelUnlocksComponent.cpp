// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Rampage/LevelUnlocksComponent.h"

#include "Pawn/BaseTankPawn.h"
#include "Item/ItemInventory.h"
#include "Item/Item.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "TankRampageLogging.h"

#include "Kismet/GameplayStatics.h"

#include <array>
#include <chrono>
#include <algorithm>
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
	IndexArray ShuffleIndices(Random& Rng, int32 Count);

	template<typename Random>
	void SelectRandomizedAvailableItems(Random& Rng, const TArray<FLevelUnlock>& PossibleUnlocks, TArray<FLevelUnlock>& OutAvailable, int32& Count);
}

// Initialize the random number generator with a seed from current time
ULevelUnlocksComponent::ULevelUnlocksComponent() : Rng(std::chrono::system_clock::now().time_since_epoch().count())
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULevelUnlocksComponent::BeginPlay()
{
	Super::BeginPlay();

	GiveLocalPlayerFirstWeapon();
}

void ULevelUnlocksComponent::GiveLocalPlayerFirstWeapon() const
{
	if (const auto& FirstLevelUnlockOpt = GetFirstLevelUnlockOptions(); FirstLevelUnlockOpt && FirstLevelUnlockOpt->Config)
	{
		auto World = GetWorld();
		check(World);

		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, FirstLevelUnlockOpt]
		{
			this->ApplyLevelUnlock(UGameplayStatics::GetPlayerPawn(this, 0), FirstLevelUnlockOpt->Config.AvailableUnlocks[0]);
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
		NumAvailableOptions = GetNumUnlockOptions(LevelUnlocks[NextLevel]);
		NumCurrent = NumAvailableOptions > 0 ? FMath::RandRange(1, NumAvailableOptions) : 0;
	}
}

TArray<FLevelUnlock> ULevelUnlocksComponent::GetAvailableUnlocks(const int32 NextLevel, const UItemInventory* ItemInventory, int32& NumCurrent, const int32 NumAvailableOptions) const
{
	TArray<FLevelUnlock> PossibleUnlocks;
	TArray<FLevelUnlock> AvailableUnlocks;

	PossibleUnlocks.Reserve(MaxOptions);
	AvailableUnlocks.Reserve(NumAvailableOptions);

	const auto& CurrentItems = ItemInventory->GetCurrentItems();

	int32 NumPrevious = NumAvailableOptions - NumCurrent;

	if (NumPrevious > 0)
	{
		// Start at previous level and iterate backwards
		for (int32 i = FMath::Min(NextLevel, LevelUnlocks.Num()) - 1; i >= 0; --i)
		{
			PopulateViableUnlockOptions(CurrentItems, LevelUnlocks[i].AvailableUnlocks, PossibleUnlocks);
		}

		SelectRandomizedAvailableItems(Rng, PossibleUnlocks, AvailableUnlocks, NumPrevious);
	}

	if (NumCurrent > 0)
	{
		PossibleUnlocks.Reset();
		// if we didn't have enough previous to offer then increase NumCurrent
		NumCurrent = NumAvailableOptions - NumPrevious;

		PopulateViableUnlockOptions(CurrentItems, LevelUnlocks[NextLevel].AvailableUnlocks, PossibleUnlocks);
		SelectRandomizedAvailableItems(Rng, PossibleUnlocks, AvailableUnlocks, NumCurrent);
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

	return GetLevelUnlocksContext(1, LevelUnlocks[0].AvailableUnlocks, LevelUnlocks[0].MaxUnlockOptions);
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
	IndexArray ShuffleIndices(Random& Rng, int32 Count)
	{
		// Should be uninitialized as we initialize it in std::iota
		IndexArray Indices;
		const auto BeginIt = Indices.begin();
		const auto EndIt = std::next(Indices.begin(), Count);

		// 0, 1, 2, 3...
		std::iota(BeginIt, EndIt, 0);
		std::shuffle(BeginIt, EndIt, Rng);

		return Indices;
	}

	template<typename Random>
	void SelectRandomizedAvailableItems(Random& Rng, const TArray<FLevelUnlock>& PossibleUnlocks, TArray<FLevelUnlock>& OutAvailable, int32& Count)
	{
		Count = FMath::Min(Count, PossibleUnlocks.Num());

		const IndexArray Indices = ShuffleIndices(Rng, Count);

		for (int32 i = 0; i < Count; ++i)
		{
			OutAvailable.Add(PossibleUnlocks[Indices[i]]);
		}
	}
}