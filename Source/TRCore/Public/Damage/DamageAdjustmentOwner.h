// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include <concepts>
#include <limits>
#include <tuple>

#include "DamageAdjustmentOwner.generated.h"

namespace TR
{
	using DamageAdjustmentFunc = TFunction<float(float /* Damage*/, const AActor* /*DamagedActor*/, const AController* /*InstigatedBy*/, const AActor* /*DamageCauser*/)>;
}

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

	void RegisterDamageAdjustment(const UObject* AdjustmentOwner, const TR::DamageAdjustmentFunc& AdjustmentFunc, int32 RegistrationOrdinal = std::numeric_limits<int32>::max());

protected:
	float CalculateAdjustedDamage(float Damage, const AActor* DamagedActor, const AController* InstigatedBy, const AActor* DamageCauser) const;

private:
	
	struct FDelegateMapKey
	{
		TR::DamageAdjustmentFunc Func;
		// Prevent dangling lambda UObject "this" captures
		TWeakObjectPtr<const UObject> Object;
		const void* Tiebreaker;
		int32 Priority;

		bool operator<(const FDelegateMapKey& Other) const;
		bool operator==(const FDelegateMapKey& Other) const;
	};

	// Needs to be mutable in order to lazily remove expired objects when CalculateAdjustedDamage is called
	mutable TSortedMap<FDelegateMapKey, uint8> DamageAdjustmentDelegateOrderSet;
};

#pragma region Inline Definitions


inline bool IDamageAdjustmentOwner::FDelegateMapKey::operator<(const IDamageAdjustmentOwner::FDelegateMapKey& Other) const
{
	return std::tie(Priority, Tiebreaker) < std::tie(Other.Priority, Other.Tiebreaker);
}

inline bool IDamageAdjustmentOwner::FDelegateMapKey::operator==(const IDamageAdjustmentOwner::FDelegateMapKey& Other) const
{ 
	return std::tie(Priority, Tiebreaker) == std::tie(Other.Priority, Other.Tiebreaker);
}

#pragma endregion Inline Definitions