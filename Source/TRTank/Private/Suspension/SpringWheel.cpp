// Fill out your copyright notice in the Description page of Project Settings.


#include "Suspension/SpringWheel.h"

#include "Components/SphereComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

// Sets default values
ASpringWheel::ASpringWheel()
{
	PrimaryActorTick.bCanEverTick = false;

	// Note that renaming the subobject itself is not supported with core redirects
	RootComponent = MassAxleConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("MassWheelConstraint"));

	// Needed to rename because details panel was blank after changing UStaticMeshComponent->USphereComponent so needed to "reset it"
	AxleComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Axle"));
	AxleComponent->SetupAttachment(MassAxleConstraint);

	WheelComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Wheel"));
	WheelComponent->SetupAttachment(AxleComponent);

	AxleWheelConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("AxleWheelConstraint"));
	AxleWheelConstraint->SetupAttachment(AxleComponent);
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
			MassAxleConstraint->SetConstrainedComponents(ParentPrimitiveComponent, NAME_None, AxleComponent, NAME_None);
			AxleWheelConstraint->SetConstrainedComponents(AxleComponent, NAME_None, WheelComponent, NAME_None);

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
