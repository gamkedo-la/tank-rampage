// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Item.generated.h"

class UItemDataAsset;

/**
 * Base class for both weapons and passive effects in the game.  
   NOTE: It is planned that this hierarchy will be replaced by classes using the Gameplay Ability System (GAS) in future.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class TRITEM_API UItem : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	bool CanBeActivated() const;

	UFUNCTION(BlueprintPure)
	float GetCooldownTimeRemaining() const;

	UFUNCTION(BlueprintCallable)
	float GetCooldownProgressPercentage() const;

	UFUNCTION(BlueprintCallable)
	bool Activate(USceneComponent* ActivationReferenceComponent, const FName& ActivationSocketName);

	void Initialize(APawn* InOwner, UItemDataAsset* InItemDataAsset);

	UFUNCTION(BlueprintPure)
	APawn* GetOwner() const;

	void IncreaseLevel();
	void SetLevel(int32 Level);
	void SetMaxLevel(int32 Level);

	UFUNCTION(BlueprintPure)
	int32 GetLevel() const;

	UFUNCTION(BlueprintPure)
	int32 GetMaxLevel() const;

protected:
	virtual bool DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName)
		PURE_VIRTUAL(UItem::DoActivation, return false;);

	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintInitialize(UItemDataAsset* InItemDataAsset);

	virtual void NativeInitialize(UItemDataAsset* InItemDataAsset) {}

	UFUNCTION(BlueprintNativeEvent)
	void OnLevelChanged(int32 NewLevel, int32 PreviousLevel);
	virtual void OnLevelChanged_Implementation(int32 NewLevel, int32 PreviousLevel) {}

protected:

	UPROPERTY(Category = "Cooldown", BlueprintReadWrite)
	float CooldownTimeSeconds{};

private:
	float LastActivationTimeSeconds{ -1.0f };
	int32 ItemLevel{ 1 };
	int32 MaxItemLevel{ 1 };

	UPROPERTY(Transient)
	TObjectPtr<APawn> Owner{};
};

#pragma region Inline Definitions

inline APawn* UItem::GetOwner() const
{
	return Owner;
}

inline void UItem::IncreaseLevel()
{
	SetLevel(ItemLevel + 1);
}

inline void UItem::SetMaxLevel(int32 Level)
{
	MaxItemLevel = Level;
}

inline int32 UItem::GetLevel() const
{
	return ItemLevel;
}

inline int32 UItem::GetMaxLevel() const
{
	return MaxItemLevel;
}

#pragma endregion Inline Definitions
