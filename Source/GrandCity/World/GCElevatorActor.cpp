#include "World/GCElevatorActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
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
	SetDoorAlpha(0.0f);
}

bool AGCElevatorActor::CallToFloor(AActor* FloorMarker)
{
	if (!IsValid(FloorMarker) || !GetWorld() || FloorMarker->GetWorld() != GetWorld()
		|| FVector::DistSquared2D(FloorMarker->GetActorLocation(), GetActorLocation()) > FMath::Square(300.0f)
		|| State == EGCElevatorState::OpeningDoors || State == EGCElevatorState::Dwelling
		|| State == EGCElevatorState::ClosingDoors || State == EGCElevatorState::Moving)
	{
		return false;
	}

	RequestedFloor = FloorMarker;
	TargetZ = FloorMarker->GetActorLocation().Z;
	if (FMath::Abs(TargetZ - GetActorLocation().Z) <= 5.0f)
	{
		State = EGCElevatorState::OpeningDoors;
	}
	else if (DoorAlpha > KINDA_SMALL_NUMBER)
	{
		State = EGCElevatorState::ClosingDoors;
	}
	else
	{
		// First open at the caller's landing so a rider can board before travel begins.
		State = EGCElevatorState::OpeningDoors;
	}
	SetActorTickEnabled(true);
	return true;
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
			if (RequestedFloor.IsValid() && FMath::Abs(TargetZ - GetActorLocation().Z) > 5.0f)
			{
				DwellRemaining = FMath::Clamp(DoorDwellSeconds, 1.0f, 8.0f);
				State = EGCElevatorState::Dwelling;
			}
			else
			{
				State = EGCElevatorState::Idle;
				RequestedFloor.Reset();
			}
		}
		break;

	case EGCElevatorState::Dwelling:
		DwellRemaining -= SafeDelta;
		if (DwellRemaining <= 0.0f)
		{
			State = EGCElevatorState::ClosingDoors;
		}
		break;

	case EGCElevatorState::ClosingDoors:
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
			State = EGCElevatorState::OpeningDoors;
		}
		break;
	}

	case EGCElevatorState::Idle:
	default:
		break;
	}
}
