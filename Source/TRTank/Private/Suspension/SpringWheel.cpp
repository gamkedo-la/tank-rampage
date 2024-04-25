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
}

// Called when the game starts or when spawned
void ASpringWheel::BeginPlay()
{
	Super::BeginPlay();

	SetupConstraint();
}

void ASpringWheel::SetupConstraint()
{
	if (auto AttachParent = GetAttachParentActor(); AttachParent)
	{
		if (auto ParentPrimitiveComponent = AttachParent->FindComponentByClass<UPrimitiveComponent>(); ParentPrimitiveComponent)
		{
			MassWheelConstraint->SetConstrainedComponents(ParentPrimitiveComponent, NAME_None, Wheel, NAME_None);
			UE_VLOG_UELOG(AttachParent, LogTRTank, Log, TEXT("%s: SetupConstraint - Attached to parent %s on %s"), *GetName(), *AttachParent->GetName(), *ParentPrimitiveComponent->GetName());
		}
		else
		{
			UE_VLOG_UELOG(AttachParent, LogTRTank, Warning, TEXT("%s: SetupConstraint - Attached to parent %s has no primitive components!"), *GetName(), *AttachParent->GetName());
		}
	}
	else
	{
		UE_VLOG_UELOG(this, LogTRTank, Warning, TEXT("%s: SetupConstraint - Is not attached to another actor!"), *GetName());
	}
}
