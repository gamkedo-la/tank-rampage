// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include <concepts>

#include "ItemInventory.generated.h"

class UWeapon;
class UPassiveEffect;
class UActivatableEffect;
class UItemDataAsset;
struct FItemConfigData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnInventoryItemAdded, const UItemInventory*, Inventory, const FName&, Name, int32, Index, const FItemConfigData&, ItemConfigData);

/*
* Contains the items that the player currently has available.  Items can be weapons like the main gun, missiles, EMP, mini nuke etc, or activatible effects like a shield or turbo speed.
* Passive effects like health and armor upgrades are handled separately as they are permantently applied to the player's attributes and do not need to be activated. 
* Item names are identified via constants in <code>ItemNames.h</code>.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TRITEM_API UItemInventory : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UItemInventory();

	UFUNCTION(BlueprintCallable)
	void Clear();

	/*
	* Sets the active weapon with the given <code>Name</code>.
	*/
	UFUNCTION(BlueprintCallable)
	void SetActiveWeaponByName(const FName& Name);

	/*
	* Sets the active weapon with the given <code>Index</code>.
	*/
	UFUNCTION(BlueprintCallable)
	void SetActiveWeaponByIndex(int32 Index);

	/*
	* Sets the active weapon. Player must already have this weapon in the inventory.
	* To add a new weapon to inventory use <code>AddItemByName</code>.
	*/
	UFUNCTION(BlueprintCallable)
	void SetActiveWeapon(UWeapon* Weapon);

	UFUNCTION(BlueprintPure)
	UWeapon* GetActiveWeapon() const;

	UFUNCTION(BlueprintPure)
	bool HasAnyActiveWeapon() const;

	UFUNCTION(BlueprintCallable)
	bool SetNextWeaponActive();

	UFUNCTION(BlueprintCallable)
	bool SetPreviousWeaponActive();

	UFUNCTION(BlueprintPure)
	int32 GetNumWeapons() const;

	/*
	* Is the <code>WeaponIndex</code> available in the inventory.
	*/
	UFUNCTION(BlueprintPure)
	bool IsWeaponAvailableByIndex(int32 WeaponIndex) const;

	/*
	* Is weapon both available and not in cooldown and ready to be activated.
	*/
	UFUNCTION(BlueprintPure)
	bool CanWeaponBeActivatedByIndex(int32 WeaponIndex) const;

	UFUNCTION(BlueprintPure)
	UItem* GetItemByName(const FName& Name) const;

	UFUNCTION(BlueprintPure)
	bool HasItem(const FName& Name) const;

	/*
	* Adds a new item with the given <code>Name</code> to the inventory if it is a valid identifier and inventory does not already have this item.
	* Returns the index of the item in the inventory or <code>INDEX_NONE</code> if it cannot be added.
	*/
	UFUNCTION(BlueprintCallable)
	int32 AddItemByName(const FName& Name);

	UFUNCTION(BlueprintPure)
	TArray<UItem*> GetCurrentItems() const;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnInventoryItemAdded OnInventoryItemAdded{};


#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

private:
	bool RotateActiveWeapon(int32 Offset);

	template<std::derived_from<UItem> T>
	int32 AddToInventoryArray(const FName& Name, const FItemConfigData& ItemConfigRow, TArray<T*>& Array);

private:

	UPROPERTY(Category = "Weapon", EditDefaultsOnly)
	TObjectPtr<UItemDataAsset> ItemDataAsset{};

	UPROPERTY(Transient)
	TArray<UWeapon*> Weapons{};

	UPROPERTY(Transient)
	TArray<UPassiveEffect*> PassiveEffects{};

	UPROPERTY(Transient)
	TArray<UActivatableEffect*> ActivatableEffects{};

	UPROPERTY(Transient)
	TMap<FName, UItem*> ItemMap{};

	int32 ActiveWeaponIndex{};
};

#pragma region Inline Defintions

inline bool UItemInventory::HasAnyActiveWeapon() const
{
	return ActiveWeaponIndex < GetNumWeapons();
}

inline bool UItemInventory::SetNextWeaponActive()
{
	return RotateActiveWeapon(1);
}

inline bool UItemInventory::SetPreviousWeaponActive()
{
	return RotateActiveWeapon(-1);
}

inline UWeapon* UItemInventory::GetActiveWeapon() const
{
	return HasAnyActiveWeapon() ? Weapons[ActiveWeaponIndex] : nullptr;
}

inline int32 UItemInventory::GetNumWeapons() const
{
	return Weapons.Num();
}

inline bool UItemInventory::IsWeaponAvailableByIndex(int32 WeaponIndex) const
{
	return ActiveWeaponIndex >= 0 && WeaponIndex < GetNumWeapons();
}

inline bool UItemInventory::HasItem(const FName& Name) const
{
	return GetItemByName(Name) != nullptr;
}

#pragma endregion Inline Definitions
