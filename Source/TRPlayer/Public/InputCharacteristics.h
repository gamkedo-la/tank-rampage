#pragma once

#include "CoreMinimal.h"

#include <atomic>

namespace TR
{
	class TRPLAYER_API FInputCharacteristics
	{
	public:
		static bool IsGamepadAvailable();
		static void SetGamepadAvailable(bool bAvailable);

	private:
		static std::atomic_bool bGamepadAvailable;
	};
}

#pragma region Inline Definitions

FORCEINLINE bool TR::FInputCharacteristics::IsGamepadAvailable()
{
	return bGamepadAvailable.load(std::memory_order::acquire);
}

FORCEINLINE void TR::FInputCharacteristics::SetGamepadAvailable(bool bAvailable)
{
	bGamepadAvailable.store(bAvailable, std::memory_order::release);
}

#pragma endregion Inline Definitions