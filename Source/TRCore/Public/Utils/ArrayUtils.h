// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Utils/StringUtils.h"

#include <concepts>
#include <array>
#include <iterator>
#include <type_traits>

namespace TR
{
	template<typename TCont, typename T>
	concept BaseContainerConcept = requires(TCont c, T t)
	{
		{
			*c.begin()
		} -> std::convertible_to<T>;
		{
			*c.end()
		} -> std::convertible_to<T>;
	};

	template<typename It, typename T>
	concept ForwardIteratorT = std::forward_iterator<It> && requires(It it)
	{
		{
			*it
		} -> std::convertible_to<T>;
	};

	template<typename Func, typename TCont, typename T>
	concept SizeFunctionConcept = BaseContainerConcept<TCont,T> && requires(Func f, const TCont& c)
	{
		{
			f(c)
		} -> std::signed_integral;
	};

	template<typename TCont, typename T> requires BaseContainerConcept<TCont,T>
	struct UEContainerSizeFunction
	{
		int32 operator()(const TCont& Cont) const { return Cont.Num(); }
	};

	template<typename T, StringUtils::ToStringFunctorConcept<T> ElementStringFunc = StringUtils::ObjectToString<T>, typename InAllocatorType>
	FString ToString(const TArray<T,InAllocatorType>& Array, ElementStringFunc Func = {});

	template<typename T, BaseContainerConcept<T> TCont, StringUtils::ToStringFunctorConcept<T> ElementStringFunc = StringUtils::ObjectToString<T>, SizeFunctionConcept<TCont, T> TSizeContFunc = UEContainerSizeFunction<TCont, T>>
	FString ToString(const TCont& Cont, ElementStringFunc Func = {}, TSizeContFunc SizeContFunc = {});

	template<std::derived_from<UObject> T, typename InAllocatorType>
	inline FString ToStringObjectElements(const TArray<T*, InAllocatorType>& Array)
	{
		return ToString(Array, StringUtils::ObjectName<T>{});
	}

	template<std::derived_from<UObject> T, BaseContainerConcept<T*> TCont>
	inline FString ToStringObjectElements(const TCont& Cont)
	{
		return ToString<T*>(Cont, StringUtils::ObjectName<T>{}, UEContainerSizeFunction<TCont, T*>{});
	}

	template<StringUtils::UEnumConcept T, typename InAllocatorType>
	inline FString ToString(const TArray<T, InAllocatorType>& Array)
	{
		return ToString(Array, StringUtils::EnumToString<T>{});
	}

	template<typename InAllocatorType>
	inline FString ToString(const TArray<FString, InAllocatorType>& Array)
	{
		return ToString(Array, [](const auto& elm) -> decltype(auto) { return elm; });
	}

	template<typename T, BaseContainerConcept<T> TCont, ForwardIteratorT<T> TIt>
	TIt StdCopy(const TCont& Cont, TIt Begin, TIt End);

	template<typename T, typename InAllocatorType, ForwardIteratorT<T> TIt>
	TIt StdArrayCopy(const TArray<T, InAllocatorType>& Array, TIt Begin, TIt End)
	{
		return StdCopy<T>(Array, Begin, End);
	}
}

/// Template Definition //////
namespace TR
{
	template<typename T, StringUtils::ToStringFunctorConcept<T> ElementStringFunc, typename InAllocatorType>
	inline FString ToString(const TArray<T, InAllocatorType>& Array, ElementStringFunc Func)
	{
		return ToString<T>(Array, Func, UEContainerSizeFunction<TArray<T, InAllocatorType>, T>{});
	}

	template<typename T, BaseContainerConcept<T> TCont, StringUtils::ToStringFunctorConcept<T> ElementStringFunc, SizeFunctionConcept<TCont, T> TSizeContFunc>
	FString ToString(const TCont& Cont, ElementStringFunc Func, TSizeContFunc SizeContFunc)
	{
		FString Output;
		Output.Reserve(SizeContFunc(Cont) * 128);

		Output.AppendChar('[');

		bool bFirst = true;

		for (const auto& Elm : Cont)
		{
			if (!bFirst)
			{
				Output.AppendChar(',');
			}
			else
			{
				bFirst = false;
			}

			Output.Append(Func(Elm));
		}

		Output.AppendChar(']');

		return Output;
	}

	template<typename T, BaseContainerConcept<T> TCont, ForwardIteratorT<T> TIt>
	TIt StdCopy(const TCont& Cont, TIt Begin, TIt End)
	{
		auto It = Begin;

		for (const auto& Elm : Cont)
		{
			if (It == End)
			{
				break;
			}

			*It++ = Elm;
		}

		return It;
	}
}
