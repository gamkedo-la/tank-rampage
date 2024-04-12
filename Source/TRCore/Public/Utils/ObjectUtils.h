// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"

#include <concepts>


namespace TR::ObjectUtils
{
	template<std::derived_from<UObject> T>
	bool IsClassDefaultObject(const T* Object);

	template<std::derived_from<UObject> T>
	T* GetClassDefaultObject();

	template<std::derived_from<UActorComponent> T>
	T* FindDefaultComponentByClass(AActor* ActorClassDefault);
}


#pragma region Template Definitions

template<std::derived_from<UObject> T>
T* TR::ObjectUtils::GetClassDefaultObject()
{
	auto Class = T::StaticClass();
	check(Class);

	auto RawCDO = Class->GetDefaultObject();
	auto CDO = Cast<T>(RawCDO);
	ensureMsgf(CDO, TEXT("CDO is incorrect type for %s -> %s"), *Class->GetName(), RawCDO ? *RawCDO->GetName() : TEXT("NULL"));

	return CDO;
}

template<std::derived_from<UObject> T>
inline bool TR::ObjectUtils::IsClassDefaultObject(const T* Object)
{
    return Object && 
            ((Object->GetFlags() & (EObjectFlags::RF_ArchetypeObject | EObjectFlags::RF_ClassDefaultObject)) ||
            (Object->GetClass() && Object->GetClass()->GetDefaultObject() == Object));
}

// Adapted from https://forums.unrealengine.com/t/how-to-get-a-component-from-a-classdefaultobject/383881/5
template<std::derived_from<UActorComponent> T>
T* TR::ObjectUtils::FindDefaultComponentByClass(AActor* ActorClassDefault)
{
    if (!ActorClassDefault)
    {
        return nullptr;
    }

    if (auto FoundComponent = ActorClassDefault->FindComponentByClass<T>(); FoundComponent)
    {
        return FoundComponent;
    }

    UClass* InActorClass = ActorClassDefault->GetClass();

    // Check blueprint nodes. Components added in blueprint editor only (and not in code) are not available from
    // CDO.
    const auto RootBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(InActorClass);
    const auto InComponentClass = T::StaticClass();

    UClass* ActorClass = InActorClass;

    // Go down the inheritance tree to find nodes that were added to parent blueprints of our blueprint graph.
    do
    {
        UBlueprintGeneratedClass* ActorBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(ActorClass);
        if (!ActorBlueprintGeneratedClass)
        {
            return nullptr;
        }

        const TArray<USCS_Node*>& ActorBlueprintNodes =
            ActorBlueprintGeneratedClass->SimpleConstructionScript->GetAllNodes();

        for (USCS_Node* Node : ActorBlueprintNodes)
        {
            if (Node->ComponentClass->IsChildOf(InComponentClass))
            {
                return Cast<T>(Node->GetActualComponentTemplate(RootBlueprintGeneratedClass));
            }
        }

        ActorClass = Cast<UClass>(ActorClass->GetSuperStruct());

    } while (ActorClass != AActor::StaticClass());

    return nullptr;
}

#pragma endregion Template Definitions
