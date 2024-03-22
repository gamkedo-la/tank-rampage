// Fill out your copyright notice in the Description page of Project Settings.


#include "Breakable/BreakableActorBase.h"

#include "GeometryCollection/GeometryCollectionComponent.h"

ABreakableActorBase::ABreakableActorBase()
{
	PrimaryActorTick.bCanEverTick = false;

	GeometryCollectionComponent = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("Geometry Collection"));
	SetRootComponent(GeometryCollectionComponent);

	GeometryCollectionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
}

