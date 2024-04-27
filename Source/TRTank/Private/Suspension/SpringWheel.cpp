// Fill out your copyright notice in the Description page of Project Settings.


#include "Suspension/SpringWheel.h"

#include "Components/SphereComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

#include "GameFramework/MovementComponent.h" 

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

ASpringWheel::ASpringWheel()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	PrimaryActorTick.TickGroup = ETickingGroup::TG_PostPhysics;

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
	CurrentForce += ForceMagnitude;

	UE_VLOG_UELOG(GetLogContext(), LogTRTank, VeryVerbose, TEXT("%s-%s: AddDrivingForce: %f"),
		*GetName(), *LoggingUtils::GetName(AttachParent), ForceMagnitude);
}

// Called when the game starts or when spawned
void ASpringWheel::BeginPlay()
{
	Super::BeginPlay();

	SetupConstraint();
	SetupTickDependencies();
}

void ASpringWheel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!FMath::IsNearlyZero(CurrentForce) && IsGrounded())
	{
		ApplyDrivingForce();
	}
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

bool ASpringWheel::IsGrounded() const
{
	auto World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(AttachParent);

	FHitResult HitResult;

	const auto& UpVector = WheelComponent->GetUpVector();
	const auto& ReferenceLocation = WheelComponent->GetComponentLocation();

	const auto TraceExtent = GroundTraceExtent + WheelComponent->GetScaledSphereRadius();

	const auto StartLocation = ReferenceLocation + UpVector * TraceExtent;
	const auto EndLocation = ReferenceLocation - UpVector * TraceExtent;

	return World->LineTraceTestByChannel(
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_Visibility,
		Params);
}

void ASpringWheel::ApplyDrivingForce()
{
	UE_VLOG_UELOG(GetLogContext(), LogTRTank, VeryVerbose, TEXT("%s-%s: ApplyDrivingForce: %f"),
		*GetName(), *LoggingUtils::GetName(AttachParent), CurrentForce);

	WheelComponent->AddForce(AxleComponent->GetForwardVector() * CurrentForce);

	CurrentForce = 0;
}

void ASpringWheel::SetupTickDependencies()
{
	if (!AttachParent)
	{
		// already logged this issue elsewhere
		return;
	}

	auto MovementComponent = AttachParent->FindComponentByClass<UMovementComponent>();

	if (MovementComponent)
	{
		// Tick after movement - note that movement is ticked after pawn player controlller which is what processes the input for the frame so will respond to throttle on same frame
		PrimaryActorTick.AddPrerequisite(MovementComponent, MovementComponent->PrimaryComponentTick);
	}
	else
	{
		UE_VLOG_UELOG(AttachParent, LogTRTank, Error, TEXT("%s-%s: Owner does not have a UMovementComponent available"), *LoggingUtils::GetName(AttachParent), *GetName());
	}
}

#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void ASpringWheel::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = FString::Printf(TEXT("Wheel (%s)"), *GetName());

	Category.Add(TEXT("Grounded"), LoggingUtils::GetBoolString(IsGrounded()));
	Category.Add(TEXT("CurrentForce"), FString::Printf(TEXT("%f"), CurrentForce));

	UE_VLOG_LOCATION(GetLogContext(), LogTRTank, Log, WheelComponent->GetComponentLocation(), WheelComponent->GetScaledSphereRadius(), FColor::Red, TEXT(""));

	// Push the category to correct nesting in the visual logger details panel
	auto& StatusArray = Snapshot->Status;
	if (StatusArray.IsEmpty())
	{
		StatusArray.Add(Category);
	}
	else
	{
		// Nest under parent
		StatusArray.Last().AddChild(Category);
	}
}

#endif

#pragma endregion Visual Logger
