// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/TRButton.h"

#include "Logging/LoggingUtils.h"
#include "TRUILogging.h"


void UTRButton::PostInitProperties()
{
	Super::PostInitProperties();

	// Make sure world is valid and it's not a PIE world
	auto World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	BindEvents();
}

void UTRButton::BeginDestroy()
{
	Super::BeginDestroy();

	if (!HoverHandle.IsValid())
	{
		return;
	}

	if (auto World = GetWorld(); World)
	{
		World->GetTimerManager().ClearTimer(HoverHandle);
	}
}

void UTRButton::BindEvents()
{
	if (HoverMaxScale > 1)
	{
		OnHovered.AddUniqueDynamic(this, &ThisClass::DoHover);
	}
}

void UTRButton::DoHover()
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	HoverStartTime = World->GetTimeSeconds() - FMath::Max(0.0f, HoverDelayTime);

	World->GetTimerManager().SetTimer(HoverHandle, this, &ThisClass::TickHover, HoverUpdateTime, true, HoverDelayTime);
}

void UTRButton::TickHover()
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	const auto TimeSeconds = World->GetTimeSeconds();
	const auto TimeElapsed = TimeSeconds - HoverStartTime;

	if (TimeElapsed >= HoverScaleTime)
	{
		SetRenderScale({ 1.0f, 1.0f });
		World->GetTimerManager().ClearTimer(HoverHandle);
		return;
	}

	// flip scaling midway through

	auto Alpha = TimeElapsed / (HoverScaleTime * 0.5);
	if (Alpha > 1)
	{
		Alpha = 1 - FMath::Fractional(Alpha);
	}

	const auto Scale = FMath::InterpEaseInOut(1.0f, HoverMaxScale, Alpha, HoverEaseFactor);
	SetRenderScale({ Scale, Scale });
}
