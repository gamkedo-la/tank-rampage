// Fill out your copyright notice in the Description page of Project Settings.


#include "Damage/DamageAdjustmentOwner.h"

#include "Logging/LoggingUtils.h"
#include "TRCoreLogging.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DamageAdjustmentOwner)

#if ENABLE_VISUAL_LOG
#define SHOULD_LOG_DAMAGE(CategoryName, Verbosity) ((FVisualLogger::IsRecording() || UE_LOG_ACTIVE(CategoryName, Verbosity)) && !DamageAdjustmentDelegateOrderSet.IsEmpty())
#else
#define SHOULD_LOG_DAMAGE(CategoryName, Verbosity) (UE_LOG_ACTIVE(CategoryName, Verbosity) && !DamageAdjustmentDelegateOrderSet.IsEmpty())
#endif

IDamageAdjustmentOwner* IDamageAdjustmentOwner::GetFromActor(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	auto DamageAdjustmentOwner = Cast<IDamageAdjustmentOwner>(Actor);
	if(DamageAdjustmentOwner)
	{
		return DamageAdjustmentOwner;
	}

	return Cast<IDamageAdjustmentOwner>(Actor->FindComponentByInterface<UDamageAdjustmentOwner>());
}

void IDamageAdjustmentOwner::RegisterDamageAdjustment(const UObject* AdjustmentOwner, const TR::DamageAdjustmentFunc& AdjustmentFunc, int32 RegistrationOrdinal)
{
	if (!ensureAlwaysMsgf(AdjustmentFunc, TEXT("AdjustmentFunc was NULL")))
	{
		return;
	}

	if (!ensureAlwaysMsgf(AdjustmentOwner, TEXT("AdjustmentOwner was NULL")))
	{
		return;
	}

	DamageAdjustmentDelegateOrderSet.FindOrAdd(FDelegateMapKey{
		.Func = AdjustmentFunc,
		.Object = AdjustmentOwner,
		.Tiebreaker = AdjustmentOwner,
		.Priority = RegistrationOrdinal
	});
}


float IDamageAdjustmentOwner::CalculateAdjustedDamage(float Damage, const AActor* DamagedActor, const AController* InstigatedBy, const AActor* DamageCauser) const
{
	check(DamagedActor);

	if (SHOULD_LOG_DAMAGE(LogTRCore, Log))
	{
		UE_VLOG_UELOG(DamagedActor, LogTRCore, Log, TEXT("%s: CalculateAdjustedDamage(%d): OriginalDamage=%f; InstigatedBy=%s; DamageCauser=%s"),
			*DamagedActor->GetName(), DamageAdjustmentDelegateOrderSet.Num(), Damage, *LoggingUtils::GetName(InstigatedBy), *LoggingUtils::GetName(DamageCauser));
	}

	auto FinalDamage = Damage;

	for (auto It = DamageAdjustmentDelegateOrderSet.CreateIterator(); It; ++It)
	{
		auto& Entry = It->Key;

		if(Entry.Object.IsValid())
		{
			// already guarded on registration - should never be null
			check(Entry.Func);
			FinalDamage = Entry.Func(FinalDamage, DamagedActor, InstigatedBy, DamageCauser);
		}
		else
		{
			// owning object deregistered or deallocated
			It.RemoveCurrent();
		}
	}

	if (SHOULD_LOG_DAMAGE(LogTRCore, Log))
	{
		UE_VLOG_UELOG(DamagedActor, LogTRCore, Log, TEXT("%s: CalculateAdjustedDamage(%d): Damage Adjusted from %f -> %f"),
			*DamagedActor->GetName(), DamageAdjustmentDelegateOrderSet.Num(), Damage, FinalDamage);
	}

	return FinalDamage;
}
