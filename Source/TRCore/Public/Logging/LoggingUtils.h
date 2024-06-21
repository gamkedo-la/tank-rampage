// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include <concepts>
#include <type_traits>

#include "Utils/StringUtils.h"

namespace LoggingUtils
{
	template<TR::StringUtils::UEnumConcept T>
	FString GetName(T Value);

	/*
	 * Returns Object->GetName if not NULL and the literal "NULL" otherwise.
	*/
	FString GetName(const UObject* Object);

	auto GetBoolString(bool Result);

	template<std::integral T>
	auto Pluralize(T value);
}

#pragma region Inline Definitions

namespace LoggingUtils
{
	template<TR::StringUtils::UEnumConcept T>
	inline FString GetName(T Value)
	{
		return UEnum::GetDisplayValueAsText(Value).ToString();
	}

	inline FString GetName(const UObject* Object)
	{
		return (TR::StringUtils::ObjectName<UObject>{})(Object);
	}

	inline auto GetBoolString(bool bResult)
	{
		return bResult ? TEXT("TRUE") : TEXT("FALSE");
	}

	template<std::integral T>
	inline auto Pluralize(T value)
	{
		return value != 1 ? TEXT("s") : TEXT("");
	}
}

#pragma endregion Inline Definitions