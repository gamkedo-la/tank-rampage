// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/TRButton.h"

#include "Logging/LoggingUtils.h"
#include "TRUILogging.h"

#include "Kismet/GameplayStatics.h"

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

void UTRButton::BindEvents()
{
	if (ClickSfx)
	{
		OnClicked.AddUniqueDynamic(this, &ThisClass::DoClick);
	}
	else
	{
		UE_LOG(LogTRUI, Warning, TEXT("%s: Has no ClickSfx!"), *GetName());
	}
	
	if (HoverSfx)
	{
		OnHovered.AddUniqueDynamic(this, &ThisClass::DoHover);
	}
	else
	{
		UE_LOG(LogTRUI, Warning, TEXT("%s: Has no HoverSfx!"), *GetName());
	}
}

void UTRButton::DoClick()
{
	PlayAudio(ClickSfx);
}

void UTRButton::DoHover()
{
	PlayAudio(HoverSfx);
}

void UTRButton::PlayAudio(USoundBase* Sound) const
{
	// Should never be nullptr since we checked before subscription
	if (!ensure(Sound))
	{
		return;
	}

	UGameplayStatics::PlaySound2D(this, Sound);
}
