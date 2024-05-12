// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/BasePickup.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"


ABasePickup::ABasePickup()
{	
	PrimaryActorTick.bCanEverTick = false;
}

void ABasePickup::BeginPlay()
{
	Super::BeginPlay();

	SetLifetimeIfApplicable();
}

void ABasePickup::RegisterOverlapEvent(UPrimitiveComponent* OverlapCheckComponent)
{
	if (!ensure(OverlapCheckComponent))
	{
		return;
	}

	OverlapCheckComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapBegin);
}

void ABasePickup::MarkForDestroy()
{
	if (bMarkedForDestroy)
	{
		return;
	}

	PlayPickupSfx();

	bMarkedForDestroy = true;

	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		Destroy();
	}));
}

void ABasePickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	auto Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	if (auto Controller = Pawn->GetController(); Controller && Controller->IsPlayerController())
	{
		OnOverlap(Pawn);
		ReceiveOnOverlap(Pawn);

		ApplyEffectToTarget(Pawn, InstantGameplayEffectClass);
	}
}

void ABasePickup::PlayPickupSfx()
{
	if (!PickupSfx)
	{
		UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s - PlayPickupSfx - No PickupSfx set"), *GetName());
		return;
	}

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s - PlayPickupSfx - Playing %s"), *GetName(), *PickupSfx->GetName());

	UGameplayStatics::PlaySound2D(GetWorld(), PickupSfx);
}

void ABasePickup::SetLifetimeIfApplicable()
{
	// bNetStartup indicates that an actor was placed in world rather than spawned so ignore placed actors
	if (bNetStartup || LifetimeAfterSpawn <= 0)
	{
		return;
	}

	SetLifeSpan(LifetimeAfterSpawn);

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s - SetLifetimeIfApplicable - Destroying after %fs"), *GetName(), LifetimeAfterSpawn);
}

void ABasePickup::ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	if (!ensure(GameplayEffectClass))
	{
		return;
	}

	auto TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetAbilitySystemComponent)
	{
		return;
	}

	auto EffectContextHandle = TargetAbilitySystemComponent->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);
	EffectContextHandle.AddOrigin(GetActorLocation());

	auto GameplayEffectSpecHandle = TargetAbilitySystemComponent->MakeOutgoingSpec(GameplayEffectClass, 1.0f, EffectContextHandle);
	check(GameplayEffectSpecHandle.Data);

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: Applying effect %s to %s"), *GetName(), *LoggingUtils::GetName(GameplayEffectClass), *LoggingUtils::GetName(Target));

	TargetAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*GameplayEffectSpecHandle.Data);
}
