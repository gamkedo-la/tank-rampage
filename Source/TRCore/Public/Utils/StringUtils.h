#pragma once

#include <compare>

#include "CoreMinimal.h"

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


#pragma endregion Inline Definitions
