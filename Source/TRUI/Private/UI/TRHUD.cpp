// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TRHUD.h"

#include "Logging/LoggingUtils.h"
#include "TRUILogging.h"

void ATRHUD::ShowHUD()
{
	Super::ShowHUD();

	UE_LOG(LogTRUI, Log, TEXT("%s: ShowHUD: %s"), *GetName(), LoggingUtils::GetBoolString(bShowHUD));

	OnToggleHUDVisibility(bShowHUD);
}

void ATRHUD::SetHUDVisible(bool bVisible)
{
	if (bVisible == bShowHUD)
	{
		return;
	}

	// Toggles the HUD
	ShowHUD();
}
