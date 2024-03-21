// Fill out your copyright notice in the Description page of Project Settings.


#include "Breakable/BreakableActorBase.h"

#include "GeometryCollection/GeometryCollectionComponent.h"

ABreakableActorBase::ABreakableActorBase()
{
	PrimaryActorTick.bCanEverTick = false;

	GeometryCollectionComponent = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("Geometry Collection"));
	SetRootComponent(GeometryCollectionComponent);

	// need overlap events for hit detection (Though if respond to with OnHitComponent then this is not necessary)
	GeometryCollectionComponent->SetGenerateOverlapEvents(true);
	GeometryCollectionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
}

