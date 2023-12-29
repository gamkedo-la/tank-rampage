// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemInventory.generated.h"

class UWeapon;
class UItem;
class UItemDataAsset;

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

	/*
	* Sets the active weapon with the given <code>Name</code>.
	*/
	UFUNCTION(BlueprintCallable)
	void SetActiveWeaponByName(const FName& Name);

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

	UFUNCTION(BlueprintPure)
	UItem* GetItemByName(const FName& Name) const;

	/*
	* Adds a new item with the given <code>Name</code> to the inventory if it is a valid identifier and inventory does not already have this item.
	*/
	UFUNCTION(BlueprintCallable)
	void AddItemByName(const FName& Name);

	UFUNCTION(BlueprintPure)
	TArray<UItem*> GetCurrentItems() const;


#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

protected:
	virtual void BeginPlay() override;

private:

	UPROPERTY(Category = "Weapon", EditDefaultsOnly)
	TObjectPtr<UItemDataAsset> ItemDataAsset{};

	UPROPERTY(Transient)
	TArray<UWeapon*> Weapons{};

	int32 ActiveWeaponIndex{};
};

#pragma region Inline Defintions

inline bool UItemInventory::HasAnyActiveWeapon() const
{
	return ActiveWeaponIndex < Weapons.Num();
}

inline UWeapon* UItemInventory::GetActiveWeapon() const
{
	return HasAnyActiveWeapon() ? Weapons[ActiveWeaponIndex] : nullptr;
}

#pragma endregion Inline Definitions
