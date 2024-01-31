#pragma once

#include "CoreMinimal.h"

namespace TR::GameplayTags
{
		inline const FName MovementBlocked(TEXT("Debuff.Movement.Block"));
		inline const FName AimBlocked(TEXT("Debuff.Aim.Block"));
		inline const FName ItemBlocked(TEXT("Debuff.Item.Block"));

		TRITEM_API bool HasExactTag(AActor* Actor, const FName& TagName);
}
