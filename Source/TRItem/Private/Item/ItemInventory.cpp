// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemInventory.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "Item/ItemDataAsset.h"
#include "Item/ItemConfigData.h"
#include "Item/Item.h"
#include "Item/Weapon.h"
#include "Item/ActivatableEffect.h"
#include "Item/PassiveEffect.h"

UItemInventory::UItemInventory()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UItemInventory::Clear()
{
	Weapons.Reset();
	ItemMap.Reset();
}

bool UItemInventory::RotateActiveWeapon(int32 Offset)
{
	if (Weapons.IsEmpty() || Offset == 0)
	{
		return false;
	}

	const int32 NewIndex = GetRotatedIndex(Offset);

	if (NewIndex == ActiveWeaponIndex)
	{
		return false;
	}

	SetActiveWeaponByIndex(NewIndex);
	return true;
}

int32 UItemInventory::GetRotatedIndex(int32 Offset) const
{
	if (Offset == 0)
	{
		return ActiveWeaponIndex;
	}

	ClampOffset(Offset);

	int32 NewIndex = ActiveWeaponIndex + Offset;
	ClampIndexInternal(NewIndex);

	return NewIndex;
}

void UItemInventory::ClampOffset(int32& Offset) const
{
	check(!Weapons.IsEmpty());

	// Pair offset down to one cycle
	if (Offset >= 0)
	{
		Offset %= Weapons.Num();
	}
	else
	{
		Offset = -(-Offset % Weapons.Num());
	}
}

void UItemInventory::ClampIndexInternal(int32& Index) const
{
	if (Index < 0)
	{
		Index += Weapons.Num();
	}
	else
	{
		Index %= Weapons.Num();
	}
}

bool UItemInventory::RotateActiveWeaponSkipCooldowns(int32 Offset)
{
	if (Weapons.IsEmpty() || Offset == 0)
	{
		return false;
	}

	int32 NewIndex = GetRotatedIndex(Offset);

	if (NewIndex == ActiveWeaponIndex)
	{
		return false;
	}

	if (!Weapons[NewIndex]->CanBeActivated())
	{
		bool bFound = false;

		for (int32 i = 0, Inc = FMath::Sign(Offset), Count = Weapons.Num() - 1; i < Count; ++i)
		{
			NewIndex += Inc;

			ClampIndexInternal(NewIndex);

			if (Weapons[NewIndex]->CanBeActivated())
			{
				bFound = true;
				break;
			}
		}

		if (!bFound || NewIndex == ActiveWeaponIndex)
		{
			return false;
		}
	}

	SetActiveWeaponByIndex(NewIndex);
	return true;
}

bool UItemInventory::CanWeaponBeActivatedByIndex(int32 WeaponIndex) const
{
	if (!IsWeaponAvailableByIndex(WeaponIndex))
	{
		return false;
	}

	auto Item = Weapons[WeaponIndex];
	if (!Item)
	{
		return false;
	}

	return Item->CanBeActivated();
}

void UItemInventory::SetActiveWeaponByName(const FName& Name)
{
	SetActiveWeapon(Cast<UWeapon>(GetItemByName(Name)));
}

void UItemInventory::SetActiveWeaponByIndex(int32 Index)
{
	if (IsWeaponAvailableByIndex(Index))
	{
		ActiveWeaponIndex = Index;
	}
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
	auto ItemResult = ItemMap.Find(Name);

	return ItemResult ? *ItemResult : nullptr;
}

int32 UItemInventory::AddItemByName(const FName& Name)
{
	if (GetItemByName(Name))
	{
		UE_VLOG_UELOG(this, LogTRItem, Warning, TEXT("%s-%s: AddItemByName: Inventory already contains Name=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *Name.ToString());

		return INDEX_NONE;
	}

	auto ItemConfigRow = GetItemConfigDataByName(Name);

	if (!ItemConfigRow)
	{
		return INDEX_NONE;
	}

	if (!ensure(ItemConfigRow->Class))
	{
		return INDEX_NONE;
	}

	auto ItemCDO = Cast<UItem>(ItemConfigRow->Class->GetDefaultObject());
	if (!ensureAlwaysMsgf(ItemCDO, TEXT("Could not get CDO as UItem for Class=%s;Name=%s"), *ItemConfigRow->Class->GetName(), *Name.ToString()))
	{
		return INDEX_NONE;
	}

	switch (ItemCDO->GetItemType())
	{
		case EItemType::Weapon:
			return AddToInventoryArray<UWeapon>(Name, *ItemConfigRow, Weapons);
		case EItemType::ActivatableEffect:
			return AddToInventoryArray<UActivatableEffect>(Name, *ItemConfigRow, ActivatableEffects);
		case EItemType::PassiveEffect:
			return AddToInventoryArray<UPassiveEffect>(Name, *ItemConfigRow, PassiveEffects);
		default:
			ensureAlwaysMsgf(true, TEXT("%s-%s: Attempted to add unsupported item with Name=%s and Class=%s"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *Name.ToString(), *LoggingUtils::GetName(ItemConfigRow->Class));
	}

	return INDEX_NONE;
}

FItemConfigData* UItemInventory::GetItemConfigDataByName(const FName& Name) const
{
	if (!ensureMsgf(ItemDataAsset, TEXT("ItemDataAsset is NULL"))
		|| !ensureMsgf(ItemDataAsset->ItemConfigDataTable, TEXT("ItemDataAsset(%s)::ItemConfigDataTable is NULL"), *ItemDataAsset->GetName())
	)
	{
		return nullptr;
	}

	auto ItemConfigDataTable = ItemDataAsset->ItemConfigDataTable;

	const auto Context = FString::Printf(TEXT("%s-%s: GetItemConfigDataByName: %s -> %s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *ItemConfigDataTable->GetName(), *Name.ToString());

	auto ItemConfigRow = ItemConfigDataTable->FindRow<FItemConfigData>(Name, Context);

	if (!ItemConfigRow)
	{
		UE_VLOG_UELOG(this, LogTRItem, Error, TEXT("%s - Could not find item"),
			*Context);
	}

	return ItemConfigRow;
}

bool UItemInventory::FindItemConfigDataByName(const FName& Name, FItemConfigData& OutItemConfigData) const
{
	if (auto ItemConfigDataPtr = GetItemConfigDataByName(Name); ItemConfigDataPtr)
	{
		OutItemConfigData = *ItemConfigDataPtr;
		return true;
	}

	return false;
}


template<std::derived_from<UItem> T>
int32 UItemInventory::AddToInventoryArray(const FName& Name, const FItemConfigData& ItemConfigRow, TArray<T*>& Array)
{
	auto Item = NewObject<T>(GetOwner(), ItemConfigRow.Class, Name);
	if (Item)
	{
		Item->Initialize(Cast<APawn>(GetOwner()), ItemConfigRow);

		int32 AddedIndex = Array.Add(Item);
		ItemMap.Add(Name, Item);

		OnInventoryItemAdded.Broadcast(this, Name, AddedIndex, ItemConfigRow);

		return AddedIndex;
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogTRItem, Error, TEXT("%s-%s: AddItemByName: Unable to create item with class=%s; Name=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *ItemConfigRow.Class, *Name.ToString());
	}

	return INDEX_NONE;
}

TArray<UItem*> UItemInventory::GetCurrentItems() const
{
	TArray<UItem*> Items;

	Items.Reserve(Weapons.Num() + ActivatableEffects.Num() + PassiveEffects.Num());

	Items.Append(Weapons);
	Items.Append(ActivatableEffects);
	Items.Append(PassiveEffects);

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

	FVisualLogStatusCategory Items;
	Items.Category = TEXT("Items");

	for (auto Item : GetCurrentItems())
	{
		if (!Item)
		{
			continue;
		}

		Items.Add(Item->GetName(), FString::Printf(TEXT("Level %d; CooldownTime=%.1fs"), Item->GetLevel(), Item->GetCooldownTimeRemaining()));
	}

	Category.AddChild(Items);


	Snapshot->Status.Add(Category);
}

#endif

