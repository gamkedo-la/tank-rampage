// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemInventory.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "Item/ItemDataAsset.h"
#include "Item/Item.h"
#include "Item/Weapon.h"
#include "Item/ItemNames.h"

UItemInventory::UItemInventory()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UItemInventory::BeginPlay()
{
	Super::BeginPlay();
}

void UItemInventory::SetActiveWeaponByName(const FName& Name)
{
	SetActiveWeapon(Cast<UWeapon>(GetItemByName(Name)));
}

void UItemInventory::SetActiveWeapon(UWeapon* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	const auto Index = Weapons.Find(Weapon);
	if (Index != INDEX_NONE)
	{
		ActiveWeaponIndex = Index;
	}
}

UItem* UItemInventory::GetItemByName(const FName& Name) const
{
	auto WeaponResult = Weapons.FindByPredicate([&](auto Weapon)
	{
		return Name == Weapon->GetName();
	});

	return WeaponResult ? *WeaponResult : nullptr;
}

void UItemInventory::AddItemByName(const FName& Name)
{
	if (!ensure(ItemDataAsset))
	{
		return;
	}

	if (GetItemByName(Name))
	{
		UE_VLOG_UELOG(this, LogTRItem, Warning, TEXT("%s-%s: AddItemByName: Inventory already contains Name=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *Name.ToString());
	}
	else if (Name == ItemNames::MainGunName && ensure(ItemDataAsset->MainGunClass))
	{
		auto Weapon = NewObject<UWeapon>(GetOwner(), ItemDataAsset->MainGunClass, ItemNames::MainGunName);
		if (Weapon)
		{
			Weapon->Initialize(Cast<APawn>(GetOwner()), ItemDataAsset);
			Weapons.Add(Weapon);
		}
		else
		{
			UE_VLOG_UELOG(GetOwner(), LogTRItem, Error, TEXT("%s-%s: AddItemByName: Unable to create weapon with class=%s"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(ItemDataAsset->MainGunClass));
		}
	}
	else
	{
		ensureAlwaysMsgf(true, TEXT("%s-%s: Attempted to add unsupported item with Name=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *Name.ToString());
	}
}

TArray<UItem*> UItemInventory::GetCurrentItems() const
{
	TArray<UItem*> Items;
	// Could do assignment here with Weapons but plan to add additional effects that will be in a separate array
	Items.Reserve(Weapons.Num());
	
	for (auto Weapon : Weapons)
	{
		if (Weapon)
		{
			Items.Add(Weapon);
		}
	}

	return Items;
}

#if ENABLE_VISUAL_LOG

void UItemInventory::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Inventory Component");

	const UWeapon* ActiveWeapon = GetActiveWeapon();

	Category.Add(TEXT("Active Weapon"),
		ActiveWeapon ? ActiveWeapon->GetClass()->GetName() : FString{ TEXT("None") });

	if (ActiveWeapon)
	{
		Category.Add(TEXT("Fire Cooldown Remaining"), FString::Printf(TEXT("%.1fs"),
			ActiveWeapon->GetCooldownTimeRemaining()));
	}

	Snapshot->Status.Add(Category);
}

#endif

