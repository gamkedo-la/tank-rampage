// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/Percentage.h"

#include "HealthComponent.generated.h"

class UItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHealthChanged, UHealthComponent*, HealthComponent, float, PreviousHealthValue, AController*, EventInstigator, AActor*, ChangeCauser);


// TODO: Move health and max health into AttributeSet as part of GAS

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TRTANK_API UHealthComponent : public UActorComponent, public IPercentage
{
	GENERATED_BODY()

public:	
	UHealthComponent();

	UFUNCTION(BlueprintPure)
	bool IsDead() const;

	UFUNCTION(BlueprintPure)
	bool IsAlive() const;

	UFUNCTION(BlueprintPure)
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure)
	float GetHealth() const;

	UFUNCTION(BlueprintPure)
	float GetMaxHealth() const;

#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void TakeDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

private:
	UFUNCTION()
	void OnItemUpgraded(UItem* Item);

	// Inherited via IPercentage - mostly redundant with GetHealth, GetMaxHealth, GethealthPercent
	// When accessing through UHealthComponent explicitly hide these members
	virtual float GetCurrentValue() const override;
	virtual float GetMaxValue() const override;

	using IPercentage::GetValuePercent;
		
public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnHealthChanged OnHealthChanged;

private:
	UPROPERTY(Category = "Health", EditDefaultsOnly)
	float MaxHealth{ 100.0f };

	float InitialMaxHealth{};

	UPROPERTY(Category = "Health", VisibleInstanceOnly)
	float Health{};

	UPROPERTY(Category = "Item", EditDefaultsOnly)
	bool bRegisterItemUpgradeEvents{};

};

#pragma region Inline Definitions

inline bool UHealthComponent::IsDead() const
{
	return !IsAlive();
}

inline bool UHealthComponent::IsAlive() const
{
	return Health > 0;
}

inline float UHealthComponent::GetHealthPercent() const
{
	return Health / MaxHealth;
}

inline float UHealthComponent::GetHealth() const
{
	return Health;
}

inline float UHealthComponent::GetMaxHealth() const
{
	return MaxHealth;
}


inline float UHealthComponent::GetCurrentValue() const
{
	return GetHealth();
}

inline float UHealthComponent::GetMaxValue() const
{
	return GetMaxHealth();
}

#pragma endregion Inline Definitions
