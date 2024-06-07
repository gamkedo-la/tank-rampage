// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/BaseTankPawn.h"

#include "Components/TankAimingComponent.h"
#include "Components/TankTurretComponent.h"
#include "Components/TankBarrelComponent.h"
#include "Components/TankTrackComponent.h"
#include "Components/TankMovementComponent.h"
#include "Components/HealthComponent.h"
#include "Components/FlippedOverCorrectionComponent.h"
#include "Components/TankEffectsComponent.h"
#include "Components/TankCollisionDetectionComponent.h"
#include "Components/TankCrashComponent.h"
#include "Components/TankEngineSoundsComponent.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Kismet/GameplayStatics.h"

#include "TankSockets.h"
#include "Item/Weapon.h"
#include "Item/ItemInventory.h"

#include "Subsystems/TankEventsSubsystem.h"

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "AbilitySystem/TRAbilitySystemComponent.h" 
#include "AbilitySystem/TRAttributeSet.h"

#include "Utils/CollisionUtils.h"

#include "TRTags.h"

#include <limits>

#include UE_INLINE_GENERATED_CPP_BY_NAME(BaseTankPawn)


DECLARE_CYCLE_STAT(TEXT("BaseTankPawn::Aim"), STAT_BaseTankPawn_Aim, STATGROUP_TRTank);

namespace
{
	FBox GetBounds(const UStaticMeshComponent& Comp, const FName* SocketName = nullptr);
	FVector GetSocketRelativeLocation(const UStaticMeshComponent& Comp, const FName& SocketName);
}

ABaseTankPawn::ABaseTankPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = TankBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankBody"));
	TankBody->SetSimulatePhysics(true);
	TankBody->SetCollisionProfileName(TR::CollisionProfile::Tank);

	TankTurret = CreateDefaultSubobject<UTankTurretComponent>(TEXT("TankTurret"));
	TankTurret->SetupAttachment(TankBody, TankSockets::TurretAttach);
	TankTurret->SetCollisionProfileName(TR::CollisionProfile::Tank);

	TankBarrel = CreateDefaultSubobject<UTankBarrelComponent>(TEXT("TankBarrel"));
	TankBarrel->SetupAttachment(TankTurret, TankSockets::GunAttach);
	TankBarrel->SetCollisionProfileName(TR::CollisionProfile::Tank);

	TankTreadRight = CreateDefaultSubobject<UTankTrackComponent>(TEXT("TankTreadRight"));
	TankTreadRight->SetupAttachment(TankBody, TankSockets::TreadRightAttach);
	TankTreadRight->SetCollisionProfileName(TR::CollisionProfile::Tank);

	TankTreadLeft = CreateDefaultSubobject<UTankTrackComponent>(TEXT("TankTreadLeft"));
	TankTreadLeft->SetupAttachment(TankBody, TankSockets::TreadLeftAttach);
	TankTreadLeft->SetCollisionProfileName(TR::CollisionProfile::Tank);

	TankTreadSideRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadSideRight"));
	TankTreadSideRight->SetupAttachment(TankBody, TankSockets::SideRightAttach);
	TankTreadSideRight->SetCollisionProfileName(TR::CollisionProfile::NoCollision);

	TankTreadSideLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadSideLeft"));
	TankTreadSideLeft->SetupAttachment(TankBody, TankSockets::SideLeftAttach);
	TankTreadSideLeft->SetCollisionProfileName(TR::CollisionProfile::NoCollision);

	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(TankBody);

	// Only enable for player controller
	CameraSpringArm->PrimaryComponentTick.bStartWithTickEnabled = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraSpringArm);

	TankAimingComponent = CreateDefaultSubobject<UTankAimingComponent>(TEXT("TankAimingComponent"));
	TankMovementComponent = CreateDefaultSubobject<UTankMovementComponent>(TEXT("TankMovement"));
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));

	AbilitySystemComponent = CreateDefaultSubobject<UTRAbilitySystemComponent>(TEXT("Ability System"));
	AbilitySystemComponent->SetIsReplicated(true);

	AttributeSet = CreateDefaultSubobject<UTRAttributeSet>(TEXT("Attribute Set"));

	ItemInventoryComponent = CreateDefaultSubobject<UItemInventory>(TEXT("Item Inventory"));

	FlippedOverCorrectionComponent = CreateDefaultSubobject<UFlippedOverCorrectionComponent>(TEXT("Flipped Over Correction Component"));
	TankEffectsComponent = CreateDefaultSubobject<UTankEffectsComponent>(TEXT("Tank Effects"));
	TankCollisionDetectionComponent = CreateDefaultSubobject<UTankCollisionDetectionComponent>(TEXT("Collision Detection"));
	TankCrashComponent = CreateDefaultSubobject<UTankCrashComponent>(TEXT("Crash"));
	TankEngineSoundsComponent = CreateDefaultSubobject<UTankEngineSoundsComponent>(TEXT("Engine Sounds"));

	Tags.Add(TR::Tags::Tank);
}

// Called when the game starts or when spawned
void ABaseTankPawn::BeginPlay()
{
	Super::BeginPlay();

	// Cannot call this in the constructor
	TankBody->SetMassOverrideInKg(NAME_None, 40000);
}

void ABaseTankPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (ensure(TankAimingComponent))
	{
		TankAimingComponent->SetTankComponents(
			{
					.Barrel = TankBarrel,
					.Turret = TankTurret
			});
	}

	if (ensure(TankMovementComponent))
	{
		TankMovementComponent->Initialize(
			{
				.LeftTrack = TankTreadLeft,
				.RightTrack = TankTreadRight
			});
	}

	if (ensureMsgf(TankCollisionDetectionComponent, TEXT("TankCollisionDetectionComponent"))
		&& ensureMsgf(TankTreadLeft, TEXT("TankTreadLeft"))
		&& ensureMsgf(TankTreadRight, TEXT("TankTreadRight")))
	{
		TankCollisionDetectionComponent->OnRelevantCollision.AddUObject(TankTreadLeft, &UTankTrackComponent::NotifyRelevantTankCollision);
		TankCollisionDetectionComponent->OnRelevantCollision.AddUObject(TankTreadRight, &UTankTrackComponent::NotifyRelevantTankCollision);
	}
}

// called on replicated clients and server
void ABaseTankPawn::NotifyControllerChanged()
{
	UE_VLOG_UELOG(this, LogTRTank, Log, TEXT("%s: NotifyControllerChanged: %s"), *GetName(), *LoggingUtils::GetName(GetController()));

	Super::NotifyControllerChanged();

	UpdateSpringArmTickEnabled();

	UpdateGameplayAbilitySystemAfterPossession(GetController());
}

float ABaseTankPawn::InternalTakePointDamage(float Damage, FPointDamageEvent const& PointDamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::InternalTakePointDamage(Damage, PointDamageEvent, EventInstigator, DamageCauser);

	return AdjustDamage(ActualDamage, EventInstigator, DamageCauser);
}

float ABaseTankPawn::InternalTakeRadialDamage(float Damage, FRadialDamageEvent const& RadialDamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::InternalTakeRadialDamage(Damage, RadialDamageEvent, EventInstigator, DamageCauser);

	return AdjustDamage(ActualDamage, EventInstigator, DamageCauser);
}

float ABaseTankPawn::AdjustDamage(float Damage, AController* EventInstigator, AActor* DamageCauser) const
{
	return CalculateAdjustedDamage(Damage, this, EventInstigator, DamageCauser);
}

void ABaseTankPawn::Kill()
{
	if (!HealthComponent)
	{
		return;
	}

	UE_VLOG_UELOG(this, LogTRTank, Log, TEXT("%s: Kill"), *GetName());

	// overkill to make sure tank doesn't survive
	UGameplayStatics::ApplyPointDamage(this, std::numeric_limits<float>::max() / 2, -GetActorForwardVector(), {}, nullptr, this, nullptr);
}

void ABaseTankPawn::NotifyKilled_Implementation()
{
	//
}

void ABaseTankPawn::UpdateSpringArmTickEnabled()
{
	const bool bIsLocalPlayerController = IsPlayerControlled() && IsLocallyControlled();

	UE_VLOG_UELOG(this, LogTRTank, Log, TEXT("%s: UpdateSpringArmTickEnabled - bIsLocalPlayerController=%s"), *GetName(),
		LoggingUtils::GetBoolString(bIsLocalPlayerController));

	// keep ticking spring component once possessed by a local player controller
	if (bIsLocalPlayerController)
	{
		check(CameraSpringArm);

		CameraSpringArm->SetComponentTickEnabled(bIsLocalPlayerController);
	}
}

void ABaseTankPawn::UpdateGameplayAbilitySystemAfterPossession(AController* NewController)
{
	UE_VLOG_UELOG(this, LogTRTank, Log, TEXT("%s: UpdateGameplayAbilitySystemAfterPossession: %s"), *GetName(), *LoggingUtils::GetName(NewController));

	if (!ensure(AbilitySystemComponent) || !NewController)
	{
		return;
	}

	const auto bIsMixed = NewController->IsPlayerController();

	AbilitySystemComponent->SetReplicationMode(bIsMixed ? EGameplayEffectReplicationMode::Mixed : EGameplayEffectReplicationMode::Minimal);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	InitializeAttributes();
}

void ABaseTankPawn::InitializeAttributes()
{
	// Init gameplay effect only needs to be done on the server since the attributes are replicated
	if (!HasAuthority() || !ensure(DefaultAttributes))
	{
		return;
	}

	auto ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	auto SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributes, 1.0f, ContextHandle);
	AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, AbilitySystemComponent);
}

bool ABaseTankPawn::CanFire() const
{
	auto World = GetWorld();
	check(World);

	if (!ItemInventoryComponent->HasAnyActiveWeapon())
	{
		return false;
	}

	return ItemInventoryComponent->GetActiveWeapon()->CanBeActivated();
}

float ABaseTankPawn::GetFireCooldownTimeRemaining() const
{
	auto ActiveWeapon = ItemInventoryComponent->GetActiveWeapon();

	if (!ActiveWeapon)
	{
		return 0;
	}

	return ActiveWeapon->GetCooldownTimeRemaining();
}

float ABaseTankPawn::GetFireCooldownProgressPercentage() const
{
	auto ActiveWeapon = ItemInventoryComponent->GetActiveWeapon();

	if (!ActiveWeapon)
	{
		return 0;
	}

	return ActiveWeapon->GetCooldownProgressPercentage();
}

void ABaseTankPawn::AimAt(const FAimingData& AimingData)
{
	SCOPE_CYCLE_COUNTER(STAT_BaseTankPawn_Aim);

	auto ActiveWeapon = ItemInventoryComponent->GetActiveWeapon();
	if (!ActiveWeapon)
	{
		return;
	}

	if (ActiveWeapon->IsLaunchable())
	{
		TankAimingComponent->AimAt(AimingData, ActiveWeapon->GetLaunchSpeed());
	}
	else
	{
		TankAimingComponent->AimAtWithNoLaunchSpeed(AimingData);
	}
}

bool ABaseTankPawn::Fire()
{
	check(TankBarrel);

	if (!CanFire())
	{
		return false;
	}

	auto ActiveWeapon = ItemInventoryComponent->GetActiveWeapon();
	check(ActiveWeapon);

	UE_VLOG_UELOG(this, LogTRTank, Log, TEXT("%s: Fire: %s"), *GetName(), *ActiveWeapon->GetName());

	return ActiveWeapon->Activate(TankBarrel, TankSockets::GunFire);
}

void ABaseTankPawn::SetLeftThrottle(float Value)
{
	check(TankTreadLeft);
	TankTreadLeft->SetThrottle(Value);
}

void ABaseTankPawn::SetRightThrottle(float Value)
{
	check(TankTreadRight);
	TankTreadRight->SetThrottle(Value);
}

FThrottleState ABaseTankPawn::GetThrottleState() const
{
	return FThrottleState
	{
		.LeftThrottle = TankTreadLeft->GetThrottle(),
		.RightThrottle = TankTreadRight->GetThrottle()
	};
}

bool ABaseTankPawn::IsGrounded() const
{
	return TankTreadLeft->IsGrounded() && TankTreadRight->IsGrounded();
}

bool ABaseTankPawn::IsAirborne() const
{
	return TankTreadLeft->IsAirborne() && TankTreadRight->IsAirborne();
}

void ABaseTankPawn::MoveForward(float Throw)
{
	check(TankMovementComponent);
	TankMovementComponent->MoveForward(Throw);
}

void ABaseTankPawn::TurnRight(float Throw)
{
	check(TankMovementComponent);
	TankMovementComponent->TurnRight(Throw);
}

void ABaseTankPawn::FellOutOfWorld(const UDamageType& dmgType)
{
	UE_VLOG_UELOG(this, LogTRTank, Display, TEXT("%s: FellOutOfWorld"), *GetName());

	Kill();
}

#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void ABaseTankPawn::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	// helpful article about the Visual Logger
	// https://benui.ca/unreal/visual-logger/

	if (!TankBody || !TankTurret || !TankTreadLeft || !TankTreadRight || !TankAimingComponent)
	{
		return;
	}

	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	const auto CurrentTimeSeconds = World->GetTimeSeconds();

	// Get reference to the current category
	const int32 CatIndex = Snapshot->Status.AddZeroed();
	FVisualLogStatusCategory& Category = Snapshot->Status[CatIndex];
	Category.Category = FString::Printf(TEXT("Tank (%s)"), *GetName());

	// TODO: Change color based on speed and stopping
	// 
	FColor BoxColor = FColor::Blue;

	// Add oriented bounded box for tank
	const auto OBB = GetBounds(*TankBody) + GetBounds(*TankTurret, &TankSockets::TurretAttach) + GetBounds(*TankTreadLeft, &TankSockets::TreadLeftAttach) + GetBounds(*TankTreadRight, &TankSockets::TreadRightAttach);
	const FVector& BoundsExtent = OBB.GetExtent();
	const FVector ZOffset = OBB.GetCenter();

	const auto& ActorTransform = GetActorTransform();
	const auto TransformMatrix = ActorTransform.ToMatrixNoScale();

	Snapshot->AddElement(OBB, TransformMatrix, LogTRTank.GetCategoryName(), ELogVerbosity::Log, BoxColor);

	// Draw barrel
	const auto& BarrelOBB = GetBounds(*TankBarrel);
	const auto& BarrelExtent = BarrelOBB.GetExtent();
	const auto& GunBarrelLocation = TankBarrel->GetComponentLocation() + TankBarrel->GetForwardVector() * BarrelExtent.X - TankBarrel->GetUpVector() * BarrelExtent.Z;
	auto BarrelTransform = TankBarrel->GetComponentTransform();
	BarrelTransform.SetTranslation(GunBarrelLocation);

	FColor BarrelColor;
	switch (TankAimingComponent->GetTankFiringStatus())
	{
		case ETankFiringStatus::Locked: BarrelColor = FColor::Red; break;
		case ETankFiringStatus::Aiming: BarrelColor = FColor::Orange; break;
		case ETankFiringStatus::Reloading: BarrelColor = FColor::Yellow; break;
		default: BarrelColor = FColor::Blue; break;
	}

	Snapshot->AddElement(BarrelOBB, BarrelTransform.ToMatrixNoScale(), LogTRTank.GetCategoryName(), ELogVerbosity::Log, BarrelColor);


	const auto MyController = GetController();
	Snapshot->AddElement(GetActorLocation() + ZOffset, LogTRTank.GetCategoryName(), ELogVerbosity::Log, BoxColor,
		FString::Printf(TEXT("%s\n%s"), *GetName(), *LoggingUtils::GetName(MyController)));

	// Forward vector
	const auto& ForwardVector = GetActorForwardVector();
	const auto FrontWorldLocation = GetActorLocation() + ForwardVector * FMath::Max(BoundsExtent.X, BoundsExtent.Y);

	Snapshot->AddArrow(FrontWorldLocation, FrontWorldLocation + ForwardVector * 100.0f, LogTRTank.GetCategoryName(), ELogVerbosity::Log, FColor::Red, TEXT("F"));

	TankMovementComponent->DescribeSelfToVisLog(Snapshot);
	HealthComponent->DescribeSelfToVisLog(Snapshot);
	ItemInventoryComponent->DescribeSelfToVisLog(Snapshot);
	TankAimingComponent->DescribeSelfToVisLog(Snapshot);
	TankTreadLeft->DescribeSelfToVisLog(Snapshot);
	TankTreadRight->DescribeSelfToVisLog(Snapshot);
	FlippedOverCorrectionComponent->DescribeSelfToVisLog(Snapshot);
}

#endif

#pragma endregion Visual Logger

namespace
{
	FBox GetBounds(const UStaticMeshComponent& Comp, const FName* SocketName)
	{
		auto Mesh = Comp.GetStaticMesh();
		if (!Mesh)
		{
			return FBox{ EForceInit::ForceInitToZero };
		}

		auto Bounds = Mesh->GetBounds();

		const FVector& BoundsExtent = Bounds.BoxExtent;
		FVector RelativeCenter(0, 0, BoundsExtent.Z);

		if (SocketName)
		{
			RelativeCenter += GetSocketRelativeLocation(Comp, *SocketName);
		}

		return FBox::BuildAABB(RelativeCenter, BoundsExtent);
	}

	FVector GetSocketRelativeLocation(const UStaticMeshComponent& Comp, const FName & SocketName)
	{
		const USceneComponent* CurrentComp = &Comp;
		while (auto ParentComp = CurrentComp->GetAttachParent())
		{
			if (!ParentComp->DoesSocketExist(SocketName))
			{
				CurrentComp = ParentComp;
				continue;
			}

			FTransform SocketTransform = ParentComp->GetSocketTransform(SocketName);
			FTransform RelativeTransform = ParentComp->GetRelativeTransform();
			return RelativeTransform.InverseTransformPosition(SocketTransform.GetLocation());
		}

		return FVector::ZeroVector;
	}
}

