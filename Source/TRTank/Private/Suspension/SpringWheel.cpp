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

void ASpringWheel::AddDrivingForce(float ForceMagnitude)
{
	UE_VLOG_UELOG(GetLogContext(), LogTRTank, VeryVerbose, TEXT("%s-%s: AddDrivingForce: %f"), *GetName(), *LoggingUtils::GetName(AttachParent));

	WheelComponent->AddForce(AxleComponent->GetForwardVector() * ForceMagnitude);
}

// Called when the game starts or when spawned
void ASpringWheel::BeginPlay()
{
	Super::BeginPlay();

	SetupConstraint();
}

void ASpringWheel::SetupConstraint()
{
	AttachParent = GetAttachParentActor();

	if (AttachParent)
	{
		if (auto ParentPrimitiveComponent = AttachParent->FindComponentByClass<UPrimitiveComponent>(); ParentPrimitiveComponent)
		{
			MassAxleConstraint->SetConstrainedComponents(ParentPrimitiveComponent, NAME_None, AxleComponent, NAME_None);
			AxleWheelConstraint->SetConstrainedComponents(AxleComponent, NAME_None, WheelComponent, NAME_None);

			UE_VLOG_UELOG(GetLogContext(), LogTRTank, Log, TEXT("%s: SetupConstraint - Attached to parent %s on %s"), *GetName(), *AttachParent->GetName(), *ParentPrimitiveComponent->GetName());
		}
		else
		{
			UE_VLOG_UELOG(GetLogContext(), LogTRTank, Warning, TEXT("%s: SetupConstraint - Attached to parent %s has no primitive components!"), *GetName(), *AttachParent->GetName());
		}
	}
	else
	{
		UE_VLOG_UELOG(GetLogContext(), LogTRTank, Warning, TEXT("%s: SetupConstraint - Is not attached to another actor!"), *GetName());
	}
}

const UObject* ASpringWheel::GetLogContext() const
{
	return AttachParent ? AttachParent.Get() : this;
}

#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void ASpringWheel::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	const auto CurrentTimeSeconds = World->GetTimeSeconds();

	// Get reference to the current category
	const int32 CatIndex = Snapshot->Status.AddZeroed();
	FVisualLogStatusCategory& Category = Snapshot->Status[CatIndex];
	Category.Category = FString::Printf(TEXT("Wheel (%s)"), *GetName());
}

#endif

#pragma endregion Visual Logger
