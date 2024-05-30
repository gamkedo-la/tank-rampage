// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#if NO_LOGGING
	DECLARE_LOG_CATEGORY_EXTERN(LogTRTank, NoLogging, NoLogging);
#elif UE_BUILD_SHIPPING
	DECLARE_LOG_CATEGORY_EXTERN(LogTRTank, Warning, Warning);
#else
	DECLARE_LOG_CATEGORY_EXTERN(LogTRTank, Display, All);
#endif

// Stat groups
DECLARE_STATS_GROUP(TEXT("TRTank"), STATGROUP_TRTank, STATCAT_Advanced);
