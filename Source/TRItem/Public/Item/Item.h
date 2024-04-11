// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Item.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Weapon UMETA(DisplayName = "Weapon"),
	ActivatableEffect UMETA(DisplayName = "Activatable Effect"),
	PassiveEffect UMETA(DisplayName = "Passive Effect"),
	Max UMETA(DisplayName = "MAX"),
};

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
	virtual bool CanBeActivated() const;

	UFUNCTION(BlueprintPure)
	float GetCooldownTimeRemaining() const;

	UFUNCTION(BlueprintCallable)
	float GetCooldownProgressPercentage() const;

	UFUNCTION(BlueprintCallable)
	bool Activate(USceneComponent* ActivationReferenceComponent, const FName& ActivationSocketName);

	UFUNCTION(BlueprintCallable)
	bool ActivateOnRootComponent();

	void Initialize(APawn* InOwner, const FItemConfigData& ItemConfigData);

	UFUNCTION(BlueprintPure)
	APawn* GetOwner() const;

	void IncreaseLevel();
	void SetLevel(int32 Level);

	UFUNCTION(BlueprintPure)
	int32 GetLevel() const;

	UFUNCTION(BlueprintPure)
	int32 GetMaxLevel() const;

	/*
	* Returns a user-friendly phrase for the item.
	*/
	UFUNCTION(BlueprintPure)
	FString GetDescription() const;

	UFUNCTION(BlueprintPure)
	EItemType GetItemType() const;

	FString ToString() const;

protected:
	virtual bool DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName) PURE_VIRTUAL(UItem::DoActivation, return false;);

	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintInitialize(const FItemConfigData& ItemConfigData);

	virtual void NativeInitialize(const FItemConfigData& ItemConfigData) {}

	/*
	* Blueprint callback to apply upgrades to the item given the <code>NewLevel</code>.
	*/
	UFUNCTION(BlueprintNativeEvent)
	void OnLevelChanged(int32 NewLevel, int32 PreviousLevel);

	/*
	* Native callback to apply upgrades to the item given the <code>NewLevel</code>.
	*/
	virtual void OnLevelChanged_Implementation(int32 NewLevel, int32 PreviousLevel) {}

	/*
	* Called before a level change event before the OnLevelChanged function is called.
	*/
	virtual void BeforeOnLevelChanged(int32 NewLevel, int32 PreviousLevel) {};
	/*
	* Called after a level change event before the OnLevelChanged function is called.
	*/
	virtual void AfterOnLevelChanged(int32 NewLevel, int32 PreviousLevel) {};

	virtual void NativeToString(FString& Str) const;

	UFUNCTION(BlueprintImplementableEvent)
	FString BlueprintToString() const;

	float GetLastActivationTimeSeconds() const;

protected:

	UPROPERTY(Category = "Cooldown", BlueprintReadWrite)
	float CooldownTimeSeconds{};

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	EItemType ItemType{ EItemType::Max };

private:
	float LastActivationTimeSeconds{ -1.0f };

	int32 ItemLevel{ 1 };

	UPROPERTY(Category = "Level", EditDefaultsOnly)
	int32 MaxItemLevel{ 1 };

	UPROPERTY(Category = "Display", EditDefaultsOnly)
	FString Description{};

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

inline int32 UItem::GetLevel() const
{
	return ItemLevel;
}

inline int32 UItem::GetMaxLevel() const
{
	return MaxItemLevel;
}

inline FString UItem::GetDescription() const
{
	return Description;
}

inline EItemType UItem::GetItemType() const
{
	return ItemType;
}

inline float UItem::GetLastActivationTimeSeconds() const
{
	return LastActivationTimeSeconds;
}

#pragma endregion Inline Definitions
