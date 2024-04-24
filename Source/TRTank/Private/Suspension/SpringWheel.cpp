// Fill out your copyright notice in the Description page of Project Settings.


#include "Suspension/SpringWheel.h"

#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

// Sets default values
ASpringWheel::ASpringWheel()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = MassWheelConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("MassWheelConstraint"));

	Wheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wheel"));
	Wheel->SetupAttachment(MassWheelConstraint);

	Mass = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mass"));
	Mass->SetupAttachment(MassWheelConstraint);
}

// Called when the game starts or when spawned
void ASpringWheel::BeginPlay()
{
	Super::BeginPlay();

	if (auto AttachParent = GetAttachParentActor(); AttachParent)
	{
		UE_VLOG_UELOG(AttachParent, LogTRTank, Log, TEXT("%s: Attached to parent %s"), *GetName(), *AttachParent->GetName());
	}
	else
	{
		UE_VLOG_UELOG(this, LogTRTank, Warning, TEXT("%s: Is not attached to another actor!"), *GetName());
	}
}
