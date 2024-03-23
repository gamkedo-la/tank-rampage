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

//void ABreakableActorBase::BeginPlay()
//{
//	Super::BeginPlay();
//
//	// TODO: Toggle EnableClustering and MaxClusterLevel based on scalability
//	// PostInitializeComponents, PreRegisterComponents is not working either
//
//	// Deactivate
//	GeometryCollectionComponent->EnableClustering = false;
//	GeometryCollectionComponent->MaxClusterLevel = 0;
//	GeometryCollectionComponent->BodyInstance.bSimulatePhysics = false;
//	GeometryCollectionComponent->BodyInstance.bStartAwake = false;
//	GeometryCollectionComponent->SetSimulatePhysics(false);
//	GeometryCollectionComponent->PutAllRigidBodiesToSleep();
//	GeometryCollectionComponent->Deactivate();
//
//	// Activate:
//	//GeometryCollectionComponent->RecreatePhysicsState();
//	//GeometryCollectionComponent->SetSimulatePhysics(true);
//
//	//GeometryCollectionComponent->RegisterAndInitializePhysicsProxy();
//	//GeometryCollectionComponent->ResetDynamicCollection
//}

