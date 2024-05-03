// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ShieldItem.h"
#include "Damage/DamageAdjustmentOwner.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"


UShieldItem::UShieldItem()
{
	bRequestsCooldownNotify = true;
}

bool UShieldItem::CanBeActivated() const
{
	// Use inherited top-level item class functionality that checks for cooldown
	// "Activation" here is used for refilling the shield and the cooldown period is the waiting time before the shield is ready again
	// So this is the reverse of how cooldown works with most other items because "Activation" is effectively engaging the cooldown period
	return UItem::CanBeActivated();
}

void UShieldItem::NativeInitialize(const FItemConfigData& ItemConfigData)
{
	auto DamageAdjustmentOwner = IDamageAdjustmentOwner::GetFromActor(GetOwner());

	if (!ensureMsgf(DamageAdjustmentOwner, TEXT("%s: unable to get IDamageAdjustmentOwner from Owner=%s"), *GetName(), *LoggingUtils::GetName(GetOwner())))
	{
		return;
	}

	CurrentValue = MaxValue;

	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: NativeInitialize: MaxValue=%f"), *GetName(), MaxValue);

	DamageAdjustmentOwner->RegisterDamageAdjustment(this, [this](float Damage, const AActor* DamagedActor, const AController* InstigatedBy, const AActor* DamageCauser)
	{
		return OnCalculateDamage(Damage, DamagedActor, InstigatedBy, DamageCauser);
	}, 0);
}

bool UShieldItem::DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName)
{
	++TimesUsed;

	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: DoActivation: TimesUsed=%d - Putting shield in cooldown for %fs and resetting CurrentValue=%f to MaxValue=%f"),
		*GetName(), TimesUsed, CooldownTimeSeconds, CurrentValue, MaxValue);

	CurrentValue = MaxValue;

	return true;
}

void UShieldItem::OnCooldownComplete()
{
	Super::OnCooldownComplete();

	PlayActivationSfx(ActivationSfx);
}

float UShieldItem::OnCalculateDamage(float Damage, const AActor* DamagedActor, const AController* InstigatedBy, const AActor* DamageCauser)
{
	UE_VLOG_UELOG(GetOuter(), LogTRItem, Verbose, TEXT("%s: OnCalculateDamage: Damage=%f; DamagedActor=%s; InstigatedBy=%s; DamageCauser=%s"),
		*GetName(), Damage, *LoggingUtils::GetName(DamagedActor), *LoggingUtils::GetName(InstigatedBy), *LoggingUtils::GetName(DamageCauser));

	if (Damage <= 0)
	{
		UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: OnCalculateDamage: Skipping zero or negative damage=%f"), *GetName(), Damage);
		return Damage;
	}

	if (!CanBeActivated())
	{
		UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: OnCalculateDamage: Shield in cooldown: %fs remaining"), *GetName(), GetCooldownTimeRemaining());
		return Damage;
	}

	FCurrentValueChangedWatcher ChangeWatcher(*this);

	float AbsorbAmount = FMath::Min(CurrentValue, Damage);

	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: OnCalculateDamage: DamageReduction=%f/%f; Shields=%f/%f"),
		*GetName(), AbsorbAmount, Damage, CurrentValue - AbsorbAmount, MaxValue);

	Damage -= AbsorbAmount;
	CurrentValue -= AbsorbAmount;

	if (FMath::IsNearlyZero(CurrentValue))
	{
		Recharge();
	}

	return Damage;
}

void UShieldItem::Recharge()
{
	// Guaranteed to succeed since it is called from OnCalculateDamage after checking if can be activated
	ActivateOnRootComponent();
}

void UShieldItem::PlayActivationSfx(USoundBase* Sound) const
{
	if (!ensure(Sound))
	{
		return;
	}

	auto ComponentOwner = GetOwner();
	if (!ensure(ComponentOwner))
	{
		return;
	}

	// The owner of the audio component is derived from the world context object and this will control the sound concurrency
	auto SpawnedAudioComponent = UGameplayStatics::SpawnSoundAttached(
		Sound,
		ComponentOwner->GetRootComponent(),
		NAME_None,
		FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset,
		true
	);

	if (!SpawnedAudioComponent)
	{
		// This is not an error condition as the component may not spawn if the sound is not audible, for example it attenuates below a threshold based on distance
		UE_VLOG_UELOG(this, LogTRItem, Log,
			TEXT("%s-%s: PlayActivationSfx - Unable to spawn audio component for sfx=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Sound->GetName());
		return;
	}

	UE_VLOG_UELOG(this, LogTRItem, Log,
		TEXT("%s-%s: PlayActivationSfx - Playing sfx=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *Sound->GetName());

	SpawnedAudioComponent->bAutoDestroy = true;
}
