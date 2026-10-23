#include "World/GCElevatorActor.h"

#include "Character/GCCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/GCHotelElevatorScreenWidget.h"
#include "UObject/ConstructorHelpers.h"

AGCElevatorActor::AGCElevatorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f / 30.0f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	CarFloor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarFloor"));
	CarFloor->SetupAttachment(SceneRoot);
	CarFloor->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
	CarFloor->SetRelativeScale3D(FVector(1.6f, 1.6f, 0.16f));
	CarFloor->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CarFloor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CarFloor->SetMobility(EComponentMobility::Movable);

	LeftDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoor"));
	LeftDoor->SetupAttachment(SceneRoot);
	LeftDoor->SetRelativeScale3D(FVector(0.70f, 0.08f, 2.30f));
	LeftDoor->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	LeftDoor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	LeftDoor->SetMobility(EComponentMobility::Movable);
	LeftDoor->SetRelativeLocation(FVector(-35.0f, 75.0f, 120.0f));

	RightDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoor"));
	RightDoor->SetupAttachment(SceneRoot);
	RightDoor->SetRelativeScale3D(FVector(0.70f, 0.08f, 2.30f));
	RightDoor->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RightDoor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RightDoor->SetMobility(EComponentMobility::Movable);
	RightDoor->SetRelativeLocation(FVector(35.0f, 75.0f, 120.0f));

	LeftCabinWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftCabinWall"));
	LeftCabinWall->SetupAttachment(SceneRoot);
	LeftCabinWall->SetRelativeScale3D(FVector(0.08f, 1.5f, 2.3f));
	LeftCabinWall->SetRelativeLocation(FVector(-75.0f, 0.0f, 120.0f));
	LeftCabinWall->SetMobility(EComponentMobility::Movable);
	LeftCabinWall->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	RightCabinWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightCabinWall"));
	RightCabinWall->SetupAttachment(SceneRoot);
	RightCabinWall->SetRelativeScale3D(FVector(0.08f, 1.5f, 2.3f));
	RightCabinWall->SetRelativeLocation(FVector(75.0f, 0.0f, 120.0f));
	RightCabinWall->SetMobility(EComponentMobility::Movable);
	RightCabinWall->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	BackCabinWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackCabinWall"));
	BackCabinWall->SetupAttachment(SceneRoot);
	BackCabinWall->SetRelativeScale3D(FVector(1.5f, 0.08f, 2.3f));
	BackCabinWall->SetRelativeLocation(FVector(0.0f, -75.0f, 120.0f));
	BackCabinWall->SetMobility(EComponentMobility::Movable);
	BackCabinWall->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	DoorSafetySensor = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorSafetySensor"));
	DoorSafetySensor->SetupAttachment(SceneRoot);
	DoorSafetySensor->SetBoxExtent(FVector(85.0f, 22.0f, 115.0f));
	DoorSafetySensor->SetRelativeLocation(FVector(0.0f, 92.0f, 120.0f));
	DoorSafetySensor->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DoorSafetySensor->SetCollisionObjectType(ECC_WorldDynamic);
	DoorSafetySensor->SetCollisionResponseToAllChannels(ECR_Ignore);
	DoorSafetySensor->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DoorSafetySensor->SetGenerateOverlapEvents(true);

	CabinDisplay = CreateDefaultSubobject<UWidgetComponent>(TEXT("CabinDisplay"));
	CabinDisplay->SetupAttachment(SceneRoot);
	CabinDisplay->SetWidgetSpace(EWidgetSpace::World);
	CabinDisplay->SetDrawSize(FVector2D(720.0f, 440.0f));
	CabinDisplay->SetRelativeScale3D(FVector(0.1f));
	CabinDisplay->SetTwoSided(true);
	CabinDisplay->SetRelativeLocation(FVector(0.0f, -62.0f, 150.0f));
	CabinDisplay->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	CabinDisplay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CabinDisplay->SetWidgetClass(UGCHotelElevatorScreenWidget::StaticClass());

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		CarFloor->SetStaticMesh(Cube.Object);
		LeftDoor->SetStaticMesh(Cube.Object);
		RightDoor->SetStaticMesh(Cube.Object);
		LeftCabinWall->SetStaticMesh(Cube.Object);
		RightCabinWall->SetStaticMesh(Cube.Object);
		BackCabinWall->SetStaticMesh(Cube.Object);
	}
}

void AGCElevatorActor::BeginPlay()
{
	Super::BeginPlay();
	BaseFloorZ = GetActorLocation().Z;
	SetDoorAlpha(0.0f);
	ScreenWidget = CabinDisplay ? Cast<UGCHotelElevatorScreenWidget>(CabinDisplay->GetUserWidgetObject()) : nullptr;
	if (ScreenWidget)
	{
		ScreenWidget->SetElevatorReadout(1, false, 1);
	}
}

bool AGCElevatorActor::CallFromLanding(AActor* CallerLanding, AActor* DestinationLanding, AGCCharacter* Passenger)
{
	const bool bBusy = State == EGCElevatorState::OpeningDoors || State == EGCElevatorState::Boarding
		|| State == EGCElevatorState::Dwelling || State == EGCElevatorState::ClosingDoors || State == EGCElevatorState::Moving;
	if (!IsValid(CallerLanding) || !IsValid(DestinationLanding) || !GetWorld()
		|| CallerLanding->GetWorld() != GetWorld() || DestinationLanding->GetWorld() != GetWorld()
		|| FVector::DistSquared2D(CallerLanding->GetActorLocation(), GetActorLocation()) > FMath::Square(300.0f)
		|| FVector::DistSquared2D(DestinationLanding->GetActorLocation(), GetActorLocation()) > FMath::Square(300.0f)
		|| FMath::IsNearlyEqual(CallerLanding->GetActorLocation().Z, DestinationLanding->GetActorLocation().Z, 5.0f)
		|| bBusy)
	{
		return false;
	}

	RequestedFloor = DestinationLanding;
	RequestedPassenger = Passenger;
	BoardingFloorZ = CallerLanding->GetActorLocation().Z;
	DestinationFloorZ = DestinationLanding->GetActorLocation().Z;
	BaseFloorZ = FMath::Min(BoardingFloorZ, DestinationFloorZ);
	if (ScreenWidget)
	{
		const int32 CallerFloor = FMath::RoundToInt((BoardingFloorZ - BaseFloorZ) / FMath::Max(100.0f, FloorHeightCm)) + 1;
		const int32 TargetFloor = FMath::RoundToInt((DestinationFloorZ - BaseFloorZ) / FMath::Max(100.0f, FloorHeightCm)) + 1;
		ScreenWidget->AnimateFloorSelection(CallerFloor, TargetFloor);
	}
	bGoingToPickupFloor = FMath::Abs(GetActorLocation().Z - BoardingFloorZ) > 5.0f;
	bAtPickupFloor = !bGoingToPickupFloor;
	TargetZ = BoardingFloorZ;

	if (bGoingToPickupFloor)
	{
		// The car travels to the caller first; the passenger must board before it serves the destination.
		State = DoorAlpha > KINDA_SMALL_NUMBER
			? EGCElevatorState::ClosingDoors : EGCElevatorState::Moving;
		if (Passenger)
		{
			Passenger->ShowSubtitle(TEXT("Elevator called — it is on the way."), 4.0f);
		}
	}
	else if (DoorAlpha > KINDA_SMALL_NUMBER)
	{
		BoardingRemaining = FMath::Clamp(BoardingTimeoutSeconds, 5.0f, 60.0f);
		State = EGCElevatorState::Boarding;
		if (Passenger)
		{
			Passenger->ShowSubtitle(TEXT("Elevator is here — step inside to ride."), 4.0f);
		}
	}
	else
	{
		State = EGCElevatorState::OpeningDoors;
	}
	SetActorTickEnabled(true);
	return true;
}

bool AGCElevatorActor::IsPassengerInsideCabin(const AGCCharacter* Passenger) const
{
	if (!Passenger)
	{
		return false;
	}

	const FVector LocalPosition = GetActorTransform().InverseTransformPosition(Passenger->GetActorLocation());
	return FMath::Abs(LocalPosition.X) <= 32.0f
		&& LocalPosition.Y >= -32.0f && LocalPosition.Y <= 20.0f
		&& LocalPosition.Z >= 45.0f && LocalPosition.Z <= 220.0f;
}

void AGCElevatorActor::SetDoorAlpha(float NewAlpha)
{
	DoorAlpha = FMath::Clamp(NewAlpha, 0.0f, 1.0f);
	const float Slide = DoorOpenDistanceCm * DoorAlpha;
	if (LeftDoor)
	{
		LeftDoor->SetRelativeLocation(FVector(-35.0f - Slide, 75.0f, 120.0f));
	}
	if (RightDoor)
	{
		RightDoor->SetRelativeLocation(FVector(35.0f + Slide, 75.0f, 120.0f));
	}
}

void AGCElevatorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float SafeDelta = FMath::Clamp(DeltaSeconds, 0.0f, 0.1f);
	const float SafeDoorSpeed = FMath::Clamp(FMath::IsFinite(DoorSpeedCmPerSecond) ? DoorSpeedCmPerSecond : 100.0f,
		40.0f, 180.0f);

	switch (State)
	{
	case EGCElevatorState::OpeningDoors:
		SetDoorAlpha(DoorAlpha + SafeDoorSpeed * SafeDelta / FMath::Max(30.0f, DoorOpenDistanceCm));
		if (DoorAlpha >= 0.999f)
		{
			SetDoorAlpha(1.0f);
			if (bAtPickupFloor && RequestedFloor.IsValid())
			{
				BoardingRemaining = FMath::Clamp(BoardingTimeoutSeconds, 5.0f, 60.0f);
				State = EGCElevatorState::Boarding;
				if (AGCCharacter* Passenger = RequestedPassenger.Get())
				{
					Passenger->ShowSubtitle(TEXT("Elevator is here — step inside to ride."), 4.0f);
				}
			}
			else if (bGoingToPickupFloor && RequestedFloor.IsValid())
			{
				// A safety reopen at the remote floor must not discard the active pickup request.
				DwellRemaining = FMath::Clamp(DoorDwellSeconds, 1.0f, 8.0f);
				State = EGCElevatorState::Dwelling;
			}
			else
			{
				State = EGCElevatorState::Idle;
				RequestedFloor.Reset();
				RequestedPassenger.Reset();
			}
		}
		break;

	case EGCElevatorState::Boarding:
	{
		AGCCharacter* Passenger = RequestedPassenger.Get();
		if (Passenger && IsPassengerInsideCabin(Passenger))
		{
			TargetZ = DestinationFloorZ;
			DwellRemaining = FMath::Clamp(DoorDwellSeconds, 1.0f, 8.0f);
			State = EGCElevatorState::Dwelling;
		}
		else
		{
			BoardingRemaining -= SafeDelta;
			if (BoardingRemaining <= 0.0f)
			{
				State = EGCElevatorState::Idle;
				RequestedFloor.Reset();
				RequestedPassenger.Reset();
				if (Passenger)
				{
					Passenger->ShowSubtitle(TEXT("Boarding timed out. The elevator will wait here."), 4.0f);
				}
			}
		}
		break;
	}

	case EGCElevatorState::Dwelling:
		DwellRemaining -= SafeDelta;
		if (DwellRemaining <= 0.0f)
		{
			State = EGCElevatorState::ClosingDoors;
		}
		break;

	case EGCElevatorState::ClosingDoors:
		if (DoorSafetySensor)
		{
			TArray<AActor*> Obstructions;
			DoorSafetySensor->GetOverlappingActors(Obstructions, APawn::StaticClass());
			if (!Obstructions.IsEmpty())
			{
				State = EGCElevatorState::OpeningDoors;
				break;
			}
		}
		SetDoorAlpha(DoorAlpha - SafeDoorSpeed * SafeDelta / FMath::Max(30.0f, DoorOpenDistanceCm));
		if (DoorAlpha <= 0.001f)
		{
			SetDoorAlpha(0.0f);
			State = EGCElevatorState::Moving;
		}
		break;

	case EGCElevatorState::Moving:
	{
		if (!RequestedFloor.IsValid())
		{
			State = EGCElevatorState::OpeningDoors;
			break;
		}
		const float SafeTravelSpeed = FMath::Clamp(FMath::IsFinite(TravelSpeedCmPerSecond) ? TravelSpeedCmPerSecond : 300.0f,
			100.0f, 600.0f);
		FVector NewLocation = GetActorLocation();
		NewLocation.Z = FMath::FInterpConstantTo(NewLocation.Z, TargetZ, SafeDelta, SafeTravelSpeed);
		SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);
		if (FMath::IsNearlyEqual(NewLocation.Z, TargetZ, 1.0f))
		{
			NewLocation.Z = TargetZ;
			SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);
			if (bGoingToPickupFloor)
			{
				bGoingToPickupFloor = false;
				bAtPickupFloor = true;
				TargetZ = BoardingFloorZ;
			}
			else
			{
				bAtPickupFloor = false;
			}
			State = EGCElevatorState::OpeningDoors;
		}
		break;
	}

	case EGCElevatorState::Idle:
	default:
		break;
	}

	if (CabinDisplay && !ScreenWidget)
	{
		ScreenWidget = Cast<UGCHotelElevatorScreenWidget>(CabinDisplay->GetUserWidgetObject());
	}
	if (ScreenWidget)
	{
		const float SafeFloorHeight = FMath::Max(100.0f, FloorHeightCm);
		const int32 CurrentFloor = FMath::RoundToInt((GetActorLocation().Z - BaseFloorZ) / SafeFloorHeight) + 1;
		const bool bIsMoving = State == EGCElevatorState::Moving;
		const int32 DestinationFloor = bIsMoving
			? FMath::RoundToInt((TargetZ - BaseFloorZ) / SafeFloorHeight) + 1 : CurrentFloor;
		ScreenWidget->SetElevatorReadout(CurrentFloor, bIsMoving, DestinationFloor);
		if (State == EGCElevatorState::Idle)
		{
			ScreenWidget->SetIdleFloorButtonLabel(CurrentFloor);
		}
	}
}
