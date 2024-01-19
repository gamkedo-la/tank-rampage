// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/HealthComponent.h"

#include "Subsystems/TankEventsSubsystem.h"

#include "TRTags.h"
#include "Item/PassiveEffect.h"
#include "Item/ItemNames.h"
#include "Item/ItemSubsystem.h"

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = InitialMaxHealth = MaxHealth;

	// TODO: Use gameplay tags
	GetOwner()->Tags.Add(TR::Tags::Alive);

	GetOwner()->OnTakeAnyDamage.AddUniqueDynamic(this, &ThisClass::TakeDamage);

	auto World = GetWorld();
	check(World);

	if (auto ItemSubsystem = World->GetSubsystem<UItemSubsystem>(); bRegisterItemUpgradeEvents && ensure(ItemSubsystem))
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s-%s: BeginPlay - Registering for item upgrade events"),
			*LoggingUtils::GetName(GetOwner()), *GetName());

		ItemSubsystem->OnItemUpgraded.AddUniqueDynamic(this, &ThisClass::OnItemUpgraded);
	}
}

void UHealthComponent::TakeDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	// TODO: If we don't switch to GAS, then would need to determine active defense bonuses here to reduce damage via tags in an FGameplayTagContainer added to an ActiveEffectsComponent or similar
	// Damage will be negative if health was restored
	const auto PreviousHealth = Health;
	Health = FMath::Clamp(Health - Damage, 0, MaxHealth);

	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s-%s: TakeDamage - Amount=%f from %s by %s health changed from %f -> %f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), Damage, *LoggingUtils::GetName(DamageCauser), *LoggingUtils::GetName(InstigatedBy), PreviousHealth, Health);

	if (FMath::IsNearlyZero(Health))
	{
		auto& Tags = GetOwner()->Tags;

		Tags.Remove(TR::Tags::Alive);
		Tags.Add(TR::Tags::Dead);

		UE_VLOG_UELOG(GetOwner(), LogTRTank, Display, TEXT("%s-%s: TakeDamage - Killed from %s by %s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(DamageCauser), *LoggingUtils::GetName(InstigatedBy));
	}

	if (!FMath::IsNearlyEqual(Health, PreviousHealth))
	{
		OnHealthChanged.Broadcast(this, PreviousHealth, InstigatedBy, DamageCauser);
	}
}

void UHealthComponent::OnItemUpgraded(UItem* Item)
{
	if (Item && Item->GetOwner() == GetOwner() && Item->GetName() == ItemNames::MaxHealth)
	{
		auto MaxHealthEffect = Cast<UPassiveEffect>(Item);
		if (!ensureMsgf(MaxHealthEffect, TEXT("Item=%s is not a UPassiveEffect"), *Item->GetName()))
		{
			return;
		}

		const auto OrigMaxHealth = MaxHealth;
		MaxHealth = InitialMaxHealth * MaxHealthEffect->GetCurrentValue();
		
		const auto bHealthChanged = !FMath::IsNearlyEqual(OrigMaxHealth, MaxHealth);

		if (bHealthChanged)
		{
			const auto PreviousHealth = Health;
			Health = MaxHealth;

			UE_VLOG_UELOG(GetOwner(), LogTRTank, Display, TEXT("%s-%s: OnItemUpgraded - MaxHealth upgraded from %f to %f"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), OrigMaxHealth, MaxHealth);

			AController* Controller{};
			if (auto Pawn = Cast<APawn>(GetOwner()); Pawn)
			{
				Controller = Pawn->GetController();
			}

			OnHealthChanged.Broadcast(this, PreviousHealth, Controller, GetOwner());
		}
	}
}

#if ENABLE_VISUAL_LOG

void UHealthComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Health Component");

	Category.Add(TEXT("Health"), FString::Printf(TEXT("%.1f"), GetHealth()));
	Category.Add(TEXT("MaxHealth"), FString::Printf(TEXT("%.1f"), GetMaxHealth()));
	Category.Add(TEXT("Health %"), FString::Printf(TEXT("%.1f"), GetHealthPercent() * 100));

	Snapshot->Status.Add(Category);
}

#endif