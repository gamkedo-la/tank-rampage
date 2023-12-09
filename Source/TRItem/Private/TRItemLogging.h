// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#if NO_LOGGING
	DECLARE_LOG_CATEGORY_EXTERN(LogTRItem, NoLogging, NoLogging);
#elif UE_BUILD_SHIPPING
	DECLARE_LOG_CATEGORY_EXTERN(LogTRItem, Warning, Warning);
#else
	DECLARE_LOG_CATEGORY_EXTERN(LogTRItem, Display, All);
#endif
