// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include <concepts>

#include "TRBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class TRCORE_API UTRBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Tank Rampage|Utils", meta = (DefaultToSelf = "WorldContextObject"))
	static bool IsRunningInEditor(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Tank Rampage|Utils")
	static FString GetProjectVersion();

	/*
	* Wraps between [Min,Max].  This version of wrap with proper behavior.  FMath::Wrap has a bug currently.
	* See https://forums.unrealengine.com/t/fmath-wrap-behavior/266612.
	*
	* We use <code>std::remove_cvref_t<T></code> to remove any const/volatile qualifiers during type deduction so that caller doesn't need to specify template argument if one happens to be "const".
	*/
	template<typename T> requires std::is_arithmetic_v<std::remove_cvref_t<T>>
	static T WrapEx(T Value, T Min, T Max);

	/*
	* Wraps an integer betwen [Min,Max].  This version of wrap with proper behavior.  FMath::Wrap has a bug currently.
	* See https://forums.unrealengine.com/t/fmath-wrap-behavior/266612.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "WrapEx (Integer)", Min = "0", Max = "100"), Category = "Math|Integer")
	static int32 Wrap(int32 Value, int32 Min, int32 Max);
};

#pragma region Template Definitions

template<typename T> requires std::is_arithmetic_v<std::remove_cvref_t<T>>
T UTRBlueprintFunctionLibrary::WrapEx(T Value, T Min, T Max)
{
	if (Min == Max)
	{
		return Min;
	}

	// If integral type + 1 to get the proper range, and if floating point then use size as the values are not discrete
	const auto Size = [&]()
	{
		if constexpr (std::is_integral_v<std::remove_cvref_t<T>>)
		{
			return Max - Min + 1;
		}
		else
		{
			return Max - Max;
		}
	}();

	auto EndVal{ Value };

	while (EndVal < Min)
	{
		EndVal += Size;
	}

	while (EndVal > Max)
	{
		EndVal -= Size;
	}

	return EndVal;
}

#pragma endregion Template Definitions

#pragma region Inline Definitions

inline int32 UTRBlueprintFunctionLibrary::Wrap(int32 Value, int32 Min, int32 Max)
{
	return WrapEx(Value, Min, Max);
}

#pragma endregion Inline Definitions
