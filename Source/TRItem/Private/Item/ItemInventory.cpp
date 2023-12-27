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

	InitWeapons();
}

void UItemInventory::InitWeapons()
{
	if (!ensure(ItemDataAsset))
	{
		return;
	}

	if (ensure(ItemDataAsset->MainGunClass))
	{
		auto Weapon = NewObject<UWeapon>(GetOwner(), ItemDataAsset->MainGunClass, ItemNames::MainGunName);
		if (Weapon)
		{
			Weapon->Initialize(Cast<APawn>(GetOwner()), ItemDataAsset);
			Weapons.Add(Weapon);
		}
		else
		{
			UE_VLOG_UELOG(this, LogTRItem, Error, TEXT("%s-%s: InitWeapons: Unable to create weapon with class=%s"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(ItemDataAsset->MainGunClass));
		}
	}
}

void UItemInventory::SetActiveWeaponIndex(int32 Index)
{
	if (Index >= 0 && Index < Weapons.Num())
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
	// TODO:
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

