// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include <concepts>
#include <limits>
#include <tuple>

#include "DamageAdjustmentOwner.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageAdjustment, float&, Damage, const AActor*, DamagedActor, const AController*, InstigatedBy, const AActor*, DamageCauser);

// This class does not need to be modified.
UINTERFACE(MinimalAPI, NotBlueprintable)
class UDamageAdjustmentOwner : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TRCORE_API IDamageAdjustmentOwner
{
	GENERATED_BODY()

public:

	static IDamageAdjustmentOwner* GetFromActor(AActor* Actor);

	template<std::invocable<FOnDamageAdjustment&> RegistrationFunc>
	void RegisterDamageAdjustment(UObject* Object, RegistrationFunc Func, int32 RegistrationOrdinal = std::numeric_limits<int32>::max());

protected:

	virtual FOnDamageAdjustment& GetOnDamageAdjustment() = 0;

private:
	
	struct FDelegateMapKey
	{
		TFunction<void(FOnDamageAdjustment&)> Func;
		void* Object;
		int32 Priority;

		bool operator<(const FDelegateMapKey& Other) const { return std::tie(Priority, Object) < std::tie(Other.Priority, Other.Object); }
		bool operator==(const FDelegateMapKey& Other) const { return std::tie(Priority, Object) == std::tie(Other.Priority, Other.Object); }
	};

	TSortedMap<FDelegateMapKey, uint8> DamageAdjustmentDelegateOrderSet;
};

#pragma region Template Definitions

template<std::invocable<FOnDamageAdjustment&> RegistrationFunc>
void IDamageAdjustmentOwner::RegisterDamageAdjustment(UObject* Object, RegistrationFunc Func, int32 RegistrationOrdinal)
{
	DamageAdjustmentDelegateOrderSet.FindOrAdd(FDelegateMapKey{
		.Func = Func,
		.Object = Object,
		.Priority = RegistrationOrdinal
	});

	FOnDamageAdjustment& DamageAdjuster = GetOnDamageAdjustment();

	// Register in correct order
	const auto ExistingRegistrations = DamageAdjuster.GetAllObjects();
	DamageAdjuster.Clear();

	for (auto It = DamageAdjustmentDelegateOrderSet.CreateIterator(); It; ++It)
	{
		auto& Entry = It->Key;

		if (Entry.Object == Object || ExistingRegistrations.Contains(Entry.Object))
		{
			Entry.Func(DamageAdjuster);
		}
		else
		{
			// was deregistered or deallocated
			It.RemoveCurrent();
		}
	}
}

#pragma endregion Template Definitions