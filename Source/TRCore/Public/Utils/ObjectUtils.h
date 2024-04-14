// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include <concepts>


namespace TR::ObjectUtils
{
	template<std::derived_from<UObject> T>
	bool IsClassDefaultObject(const T* Object);

	template<std::derived_from<UObject> T>
	T* GetClassDefaultObject();
}


#pragma region Template Definitions

template<std::derived_from<UObject> T>
T* TR::ObjectUtils::GetClassDefaultObject()
{
	auto Class = T::StaticClass();
	check(Class);

	auto RawCDO = Class->GetDefaultObject();
	auto CDO = Cast<T>(RawCDO);
	ensureMsgf(CDO, TEXT("CDO is incorrect type for %s -> %s"), *Class->GetName(), RawCDO ? *RawCDO->GetName() : TEXT("NULL"));

	return CDO;
}

template<std::derived_from<UObject> T>
inline bool TR::ObjectUtils::IsClassDefaultObject(const T* Object)
{
	return Object && Object->GetClass() && Object->GetClass()->GetDefaultObject() == Object;
}

#pragma endregion Template Definitions
