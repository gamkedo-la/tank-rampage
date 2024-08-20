// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/TRButton.h"

#include "Logging/LoggingUtils.h"
#include "TRUILogging.h"

#include "Widgets/Input/SButton.h"

#include "Subsystems/RealtimeTimerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TRButton)

namespace
{
	FPointerEvent CreateMouseClick(const FKey& Key);
}

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

	UnregisterTimer();
}

void UTRButton::BindEvents()
{
	if (HoverMaxScale > 1)
	{
		OnHovered.AddUniqueDynamic(this, &ThisClass::DoHover);
	}
}

void UTRButton::Hover()
{
	if (MyButton.IsValid())
	{
		MyButton->OnMouseEnter(MyButton->GetCachedGeometry(), {});
	}
	else
	{
		SlateHandleHovered();
	}
}

void UTRButton::ExitHover()
{
	if (MyButton.IsValid())
	{
		MyButton->OnMouseLeave({});
	}
	else
	{
		SlateHandleUnhovered();
	}
}

void UTRButton::DoHover()
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	HoverStartTime = World->GetRealTimeSeconds() - FMath::Max(0.0f, HoverDelayTime);

	if (auto TimerSystem = World->GetSubsystem<URealtimeTimerSubsystem>(); ensureMsgf(IsValid(TimerSystem), TEXT("URealtimeTimerSubsystem is NULL")))
	{
		TimerSystem->RealTimeTimerDelegate.AddUObject(this, &ThisClass::TickHover);
	}
}

void UTRButton::Click()
{
	if (MyButton.IsValid())
	{
		MyButton->OnMouseButtonDown(MyButton->GetCachedGeometry(), 
			CreateMouseClick(EKeys::LeftMouseButton));
		MyButton->OnMouseButtonUp(MyButton->GetCachedGeometry(),
			CreateMouseClick(EKeys::LeftMouseButton));
	}

	// OnClick handlers not called by the mouse events - probably because the events need to be delayed
	SlateHandleClicked();
}

void UTRButton::TickHover(float DeltaTime)
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	const auto TimeSeconds = World->GetRealTimeSeconds();
	const auto TimeElapsed = TimeSeconds - HoverStartTime;

	if (TimeElapsed >= HoverScaleTime)
	{
		SetRenderScale({ 1.0f, 1.0f });
		UnregisterTimer();

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

void UTRButton::UnregisterTimer()
{
	if (!HoverHandle.IsValid())
	{
		return;
	}

	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	if (auto TimerSystem = World->GetSubsystem<URealtimeTimerSubsystem>(); IsValid(TimerSystem))
	{
		TimerSystem->RealTimeTimerDelegate.Remove(HoverHandle);
	}
}

namespace
{
	inline FPointerEvent CreateMouseClick(const FKey& Key)
	{
		return { 0, { 0.f,0.f }, { 0.f,0.f }, {}, Key, 0, {} };
	}
}
