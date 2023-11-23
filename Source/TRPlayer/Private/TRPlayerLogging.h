// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#if NO_LOGGING
	DECLARE_LOG_CATEGORY_EXTERN(LogTRPlayer, NoLogging, NoLogging);
#elif UE_BUILD_SHIPPING
	DECLARE_LOG_CATEGORY_EXTERN(LogTRPlayer, Warning, Warning);
#else
	DECLARE_LOG_CATEGORY_EXTERN(LogTRPlayer, Display, All);
#endif
