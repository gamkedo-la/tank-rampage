// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemInventory.generated.h"

class UWeapon;
class UItem;
class UItemDataAsset;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TRITEM_API UItemInventory : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UItemInventory();

	UFUNCTION(BlueprintCallable)
	void SetActiveWeaponIndex(int32 Index);

	UWeapon* GetActiveWeapon() const;
	bool HasAnyActiveWeapon() const;

	UItem* GetItemByName(const FName& Name);
	void AddItemByName(const FName& Name);


#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

protected:
	virtual void BeginPlay() override;

private:
	void InitWeapons();

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
