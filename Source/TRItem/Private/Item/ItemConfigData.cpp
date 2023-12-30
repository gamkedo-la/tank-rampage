// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemConfigData.h"

#include "Item/Item.h"
#include "Logging/LoggingUtils.h"

FString FItemConfigData::ToString() const
{
    return LoggingUtils::GetName(Class);
}
