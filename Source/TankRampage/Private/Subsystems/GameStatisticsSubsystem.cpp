// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/GameStatisticsSubsystem.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "TankRampageLogging.h"
#include "Utils/ArrayUtils.h"

#include "Subsystems/TankEventsSubsystem.h"
#include "Pawn/BaseTankPawn.h"

#include "Components/HealthComponent.h"

#include "Kismet/GameplayStatics.h"

#include "Item/Item.h"
#include "Item/Weapon.h"

#include "Item/ItemInventory.h"
#include "Projectile.h"

#include "AbilitySystem/TRGameplayTags.h"
#include "GameplayTagContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameStatisticsSubsystem)

namespace
{
	const FName FreezeTagName = TR::GameplayTags::MovementBlocked;
}

void UGameStatisticsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Register delegates
	if (auto TankEventsSubsystem = InWorld.GetSubsystem<UTankEventsSubsystem>();
		ensureMsgf(TankEventsSubsystem, TEXT("%s: TankEventsSubsystem is NULL"), *GetName()))
	{
		TankEventsSubsystem->OnTankDestroyed.AddDynamic(this, &ThisClass::OnTankDestroyed);
	}

	// Get player pawn and listen for inventory updates
	auto PlayerPawn = Cast<ABaseTankPawn>(UGameplayStatics::GetPlayerPawn(&InWorld, 0));
	if (PlayerPawn)
	{
		if (auto Inventory = PlayerPawn->GetItemInventory(); ensure(Inventory))
		{
			Inventory->OnInventoryItemAdded.AddUniqueDynamic(this, &ThisClass::OnInventoryItemAdded);
		}
	}
	else
	{
		UE_LOG(LogTankRampage, Error, TEXT("%s: PlayerPawn not available - Will not be able to associate non-projectile weapons"),
			*GetName());
	}
}

void UGameStatisticsSubsystem::GetItemMostKillsAttributedTo(UItem*& Item, int32& Count) const
{
	Item = nullptr;
	Count = 0;

	for (const auto& [CurrentItem, ItemCount] : ItemKills)
	{
		if (ItemCount > Count)
		{
			Count = ItemCount;
			Item = CurrentItem;
		}
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

	AttributeEnemyKillItem(DestroyedTank, DestroyedBy, DestroyedWith);
}

void UGameStatisticsSubsystem::OnItemGameplayTagsChanged(UItem* Item, const TArray<APawn*>& AffectedPawns, const FGameplayTagContainer& Tags, bool bAdded)
{
	check(Item);

	auto FreezeGameplayTag = TR::GameplayTags::GetTagByName(FreezeTagName);
	if (!ensureMsgf(FreezeGameplayTag, TEXT("%s: OnItemGameplayTagsChanged: %s tag does not exist"), *GetName(), *FreezeTagName.ToString()))
	{
		return;
	}

	// Check if it is the tag we care about
	if (!Tags.HasTag(*FreezeGameplayTag))
	{
		UE_VLOG_UELOG(Item->GetOwner(), LogTankRampage, Verbose, TEXT("%s: OnItemGameplayTagsChanged: Item=%s; AffectedPawns=%s; Tags=%s; bAdded=%s"),
			*GetName(),
			*Item->GetName(),
			AffectedPawns.Num(),
			*Tags.ToStringSimple(),
			LoggingUtils::GetBoolString(bAdded)
		);

		return;
	}

	if (bAdded)
	{
		DisabledItemWeaponCredit = Item;
		DisabledEnemies.Append(AffectedPawns);

		UE_VLOG_UELOG(Item->GetOwner(), LogTankRampage, Verbose, TEXT("%s: OnItemGameplayTagsChanged: Item=%s; AffectedPawns=%s; DisabledEnemies=%s; Tags=%s; bAdded=%s"),
			*GetName(),
			*Item->GetName(),
			*TR::ToStringObjectElements(AffectedPawns),
			*TR::ToStringObjectElements<APawn>(DisabledEnemies),
			*Tags.ToStringSimple(),
			LoggingUtils::GetBoolString(bAdded)
		);
	}
	else
	{
		for (auto Pawn : AffectedPawns)
		{
			DisabledEnemies.Remove(Pawn);
		}

		if (AffectedPawns.IsEmpty())
		{
			DisabledItemWeaponCredit = nullptr;
		}

		UE_VLOG_UELOG(Item->GetOwner(), LogTankRampage, Verbose, TEXT("%s: OnItemGameplayTagsChanged: Item=%s; AffectedPawns=%s; DisabledEnemies=%s; Tags=%s; bAdded=%s"),
			*GetName(),
			*Item->GetName(),
			*TR::ToStringObjectElements(AffectedPawns),
			*TR::ToStringObjectElements<APawn>(DisabledEnemies),
			*Tags.ToStringSimple(),
			LoggingUtils::GetBoolString(bAdded)
		);
	}
}

void UGameStatisticsSubsystem::OnInventoryItemAdded(const UItemInventory* Inventory, const FName& Name, int32 Index, const FItemConfigData& ItemConfigData)
{
	UE_VLOG_UELOG(Inventory->GetOwner(), LogTankRampage, Log, TEXT("%s: OnInventoryItemAdded: Name=%s; Index=%d"),
		*GetName(),
		*Name.ToString(),
		Index
	);

	auto Item = Inventory->GetItemByName(Name);
	if(!ensureMsgf(Item, TEXT("%s: Inventory->GetItemByName(%s) IS NULL in OnInventoryItemAdded"), *GetName(), *Name.ToString()))
	{
		return;
	}

	Item->OnItemGameplayTagsChanged.AddUObject(this, &ThisClass::OnItemGameplayTagsChanged);
}

bool UGameStatisticsSubsystem::ShouldRecordUpdate() const
{
	auto PlayerTank = Cast<ABaseTankPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

	return PlayerTank && PlayerTank->GetHealthComponent()->IsAlive();
}

void UGameStatisticsSubsystem::AttributeEnemyKillItem(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith)
{
	// Attribute the passive weapon if disabled at time of kill
	UItem* AttributedItem{};

	if (DisabledItemWeaponCredit && DisabledEnemies.Contains(DestroyedTank))
	{
		AttributedItem = DisabledItemWeaponCredit;
	}
	else if(auto Projectile = Cast<AProjectile>(DestroyedWith); Projectile)
	{
		AttributedItem = Projectile->GetFiredFrom();
	}
	else
	{
		UE_VLOG_UELOG(DestroyedBy, LogTankRampage, Warning, TEXT("%s: AttributeEnemyKillItem: Could not determine item - DestroyedTank=%s; DestroyedWith=%s"),
			*GetName(),
			*LoggingUtils::GetName(DestroyedTank),
			*LoggingUtils::GetName(DestroyedWith)
		);

		return;
	}

	if (!ensureMsgf(AttributedItem, TEXT("DestroyedTank=%s; DestroyedWith=%s"), *LoggingUtils::GetName(DestroyedTank), *LoggingUtils::GetName(DestroyedWith)))
	{
		return;
	}

	[[maybe_unused]]
	const auto TotalKills = ++ItemKills.FindOrAdd(AttributedItem);

	UE_VLOG_UELOG(DestroyedBy, LogTankRampage, Log, TEXT("%s: AttributeEnemyKillItem: DestroyedTank=%s; DestroyedWith=%s; AttributedItem=%s; KillCount=%d"),
		*GetName(),
		*LoggingUtils::GetName(DestroyedTank),
		*LoggingUtils::GetName(DestroyedWith),
		*LoggingUtils::GetName(AttributedItem),
		TotalKills
	);
}
