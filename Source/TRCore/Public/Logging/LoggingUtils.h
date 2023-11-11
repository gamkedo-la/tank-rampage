// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

namespace LoggingUtils
{
	/*
	 * Returns Object->GetName if not NULL and the literal "NULL" otherwise.
	*/
	FString GetName(const UObject* Object);

	auto GetBoolString(bool Result);
}

#pragma region Inline Definitions

namespace LoggingUtils
{
	inline FString GetName(const UObject* Object)
	{
		return Object ? Object->GetName() : FString{ "NULL" };
	}

	inline auto GetBoolString(bool bResult)
	{
		return bResult ? TEXT("TRUE") : TEXT("FALSE");
	}
}

#pragma endregion Inline Definitions