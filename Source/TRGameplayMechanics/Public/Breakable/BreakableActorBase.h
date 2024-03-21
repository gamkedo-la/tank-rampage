// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BreakableActorBase.generated.h"

class UGeometryCollectionComponent;

UCLASS(Abstract)
class TRGAMEPLAYMECHANICS_API ABreakableActorBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ABreakableActorBase();


protected:
	UPROPERTY(Category = "Fracture", VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UGeometryCollectionComponent> GeometryCollectionComponent{};
};
