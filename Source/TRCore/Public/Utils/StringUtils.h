#pragma once

#include <compare>
#include <concepts>

#include <optional>

#include "CoreMinimal.h"

namespace TR::StringUtils
{
	template<typename T>
	concept UEnumConcept = std::is_enum_v<T> &&
		requires(T Value)
	{
		{
			UEnum::GetDisplayValueAsText(Value).ToString()
		} -> std::convertible_to<FString>;
	};

	template<typename Func, typename T>
	concept ToStringFunctorConcept = requires(const T t, Func f)
	{
		{
			f(t)
		} -> std::convertible_to<FString>;
	};

	template<typename T>
	concept ToStringConcept = requires(const T t)
	{
		{
			t.ToString()
		} -> std::convertible_to<FString>;
	};

	template<typename T> requires ToStringConcept<T>
	struct ObjectToString
	{
		FString operator()(const T* obj) const
		{
			return obj ? obj->ToString() : TEXT("NULL");
		}

		FString operator()(const T& obj) const
		{
			return obj.ToString();
		}
	};

	template<typename T> requires std::derived_from<T,UObject>
	struct ObjectName
	{
		FString operator()(const T* obj) const
		{
			return obj ? obj->GetName() : TEXT("NULL");
		}

		FString operator()(const T& obj) const
		{
			return obj.GetName();
		}
	};

	template<typename T>
	concept ConvertibleToUObject = requires(T * t)
	{
		{
			Cast<UObject>(t)
		} -> std::convertible_to<UObject*>;
	};

	template<ConvertibleToUObject T>
	struct UObjectInterfaceToString
	{
		FString operator()(const T* interfacePtr) const
		{
			auto obj = Cast<const UObject>(interfacePtr);
			return obj ? obj->GetName() : TEXT("NULL");
		}
	};

	template<typename T> requires UEnumConcept<T>
	struct EnumToString
	{
		FString operator()(const T& obj) const
		{
			return UEnum::GetDisplayValueAsText(obj).ToString();
		}
	};

	template<typename T>
	concept OptionalToStringConcept = ToStringConcept<T> || UEnumConcept<T>;

	template<ToStringConcept T>
	FString ToString(const T* Value);

	template<OptionalToStringConcept T>
	FString ToString(const std::optional<T>& Value);

	template<OptionalToStringConcept T>
	FString ToString(const TOptional<T>& Value);

	template<ToStringConcept T>
	FString ToString(const T& Value);

	template<UEnumConcept T>
	FString ToString(const T& Value);

	template<typename T> requires OptionalToStringConcept<T>
	struct OptionalToString
	{
		FString operator()(const std::optional<T>& OptionalObject) const
		{
			return OptionalObject ? ToString(*OptionalObject) : TEXT("NULL");
		}

		FString operator()(const TOptional<T>& OptionalObject) const
		{
			return OptionalObject ? ToString(*OptionalObject) : TEXT("NULL");
		}
	};
}

auto operator<=>(const FName& First, const FName& Second);

#pragma region Inline Definitions

inline auto operator<=>(const FName& First, const FName& Second)
{
	const auto Result = First.Compare(Second);

	if (Result == 0)
	{
		return std::strong_ordering::equal;
	}
	if (Result < 0)
	{
		return std::strong_ordering::less;
	}
	
	return std::strong_ordering::greater;
}

namespace TR::StringUtils
{
	template<ToStringConcept T>
	inline FString ToString(const T* Value)
	{
		return ObjectToString<T>{}(Value);
	}

	template<ToStringConcept T>
	inline FString ToString(const T& Value)
	{
		return ObjectToString<T>{}(Value);
	}

	template<UEnumConcept T>
	inline FString ToString(const T& Value)
	{
		return EnumToString<T>{}(Value);
	}

	template<OptionalToStringConcept T>
	inline FString ToString(const std::optional<T>& Value)
	{
		return OptionalToString<T>{}(Value);
	}

	template<OptionalToStringConcept T>
	inline FString ToString(const TOptional<T>& Value)
	{
		return OptionalToString<T>{}(Value);
	}
}

#pragma endregion Inline Definitions
