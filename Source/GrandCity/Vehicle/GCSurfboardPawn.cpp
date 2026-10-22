#include "Vehicle/GCSurfboardPawn.h"
#include "Camera/CameraComponent.h"
#include "Character/GCCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "World/GCOceanManager.h"

AGCSurfboardPawn::AGCSurfboardPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(Root);
	InteractionBox->SetBoxExtent(FVector(260.0f, 180.0f, 160.0f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionObjectType(ECC_Pawn);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);

	BoardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoardMesh"));
	BoardMesh->SetupAttachment(Root);
	BoardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoardMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -8.0f));
	BoardMesh->SetRelativeScale3D(FVector(3.2f, 0.75f, 0.10f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BoardMesh->SetStaticMesh(CubeMesh.Object); // fallback board; assign a real board/raft mesh in a BP child
	}

	StandingPoint = CreateDefaultSubobject<USceneComponent>(TEXT("StandingPoint"));
	StandingPoint->SetupAttachment(Root);
	StandingPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 95.0f));

	ChaseArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ChaseArm"));
	ChaseArm->SetupAttachment(Root);
	ChaseArm->SetRelativeLocation(FVector(0.0f, 0.0f, 160.0f));
	ChaseArm->TargetArmLength = 620.0f;
	ChaseArm->bUsePawnControlRotation = true;
	ChaseArm->bEnableCameraLag = true;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(ChaseArm);
	ChaseCamera->FieldOfView = 72.0f;
}

FString AGCSurfboardPawn::GetModeText() const
{
	switch (Mode)
	{
	case EGCSurfMode::Paddleboard: return TEXT("PADDLEBOARD");
	case EGCSurfMode::WhitewaterRaft: return TEXT("RAFT");
	default: return TEXT("SURFBOARD");
	}
}

bool AGCSurfboardPawn::BeginRide(AGCCharacter* Character)
{
	if (!Character || bOccupied)
	{
		return false;
	}
	if (FVector::Distance(GetActorLocation(), Character->GetActorLocation()) > 520.0f)
	{
		return false;
	}
	Rider = Character;
	bOccupied = true;
	HeadingYaw = GetActorRotation().Yaw;
	PlanarVelocity = FVector::ZeroVector;
	Balance = 0.0f;
	AController* CharController = Character->GetController();
	Character->BeginVehicleTransition();
	Character->SetActorLocation(StandingPoint->GetComponentLocation());
	Character->CompleteEnterVehicle(this, StandingPoint);
	if (CharController)
	{
		CharController->Possess(this);
	}
	return true;
}

void AGCSurfboardPawn::EndRide(bool bWipeout)
{
	if (!bOccupied || !Rider)
	{
		return;
	}
	AController* SurfController = GetController();
	const FVector ExitLocation = GetActorLocation() + GetActorRightVector() * (bWipeout ? -180.0f : 180.0f) + FVector(0, 0, 110.0f);
	Rider->BeginVehicleTransition();
	Rider->CompleteExitVehicle(ExitLocation);
	if (bWipeout)
	{
		Rider->LaunchCharacter(PlanarVelocity * 0.65f + FVector(0, 0, 280.0f), false, false);
		Rider->ShowSubtitle(TEXT("Wipeout!"), 2.0f);
	}
	if (SurfController)
	{
		SurfController->Possess(Rider);
	}
	Rider = nullptr;
	bOccupied = false;
	InputThrottle = 0.0f;
	InputSteer = 0.0f;
	PlanarVelocity = FVector::ZeroVector;
	Balance = 0.0f;
}

bool AGCSurfboardPawn::IsBeached(const FVector& Location, float SurfaceZ) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SurfBeachCheck), false, this);
	if (Rider)
	{
		Params.AddIgnoredActor(Rider);
	}
	if (GetWorld()->LineTraceSingleByChannel(Hit, Location + FVector(0, 0, 450.0f),
		Location - FVector(0, 0, 450.0f), ECC_Visibility, Params))
	{
		if (Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("Water")))
		{
			return false;
		}
		return Hit.ImpactPoint.Z > SurfaceZ - 18.0f;
	}
	return false;
}

void AGCSurfboardPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bOccupied)
	{
		UpdateRide(DeltaSeconds);
	}
	else if (GetWorld())
	{
		// Idle bob so beached boards still look alive at the shoreline.
		const float Time = GetWorld()->GetTimeSeconds();
		const float Surface = WaterBaseLevelZ + AGCOceanManager::GetWaveHeightAt(
			FVector2D(GetActorLocation().X, GetActorLocation().Y), Time, 1.0f);
		FVector Loc = GetActorLocation();
		if (Loc.Z < Surface + 30.0f)
		{
			Loc.Z = FMath::FInterpTo(Loc.Z, Surface + 6.0f, DeltaSeconds, 2.0f);
			SetActorLocation(Loc);
		}
	}
}

void AGCSurfboardPawn::UpdateRide(float DeltaSeconds)
{
	if (!GetWorld() || DeltaSeconds <= 0.0f)
	{
		return;
	}
	const float Time = GetWorld()->GetTimeSeconds();
	const FVector Loc = GetActorLocation();
	const FGCOceanSample Sample = AGCOceanManager::SampleOceanStatic(FVector2D(Loc.X, Loc.Y), Time, 1.0f);
	const float SurfaceZ = WaterBaseLevelZ + Sample.Height;
	FoamFactor = Sample.FoamFactor;

	float ModeMaxSpeed = MaxSpeed;
	float ModePaddle = PaddleAccel;
	float Stability = 1.0f;
	float Drag = 0.55f;
	switch (Mode)
	{
	case EGCSurfMode::Paddleboard:
		ModeMaxSpeed = 950.0f; ModePaddle = 720.0f; Stability = 2.4f; Drag = 0.75f;
		break;
	case EGCSurfMode::WhitewaterRaft:
		ModeMaxSpeed = 1500.0f; ModePaddle = 430.0f; Stability = 1.7f; Drag = 0.45f;
		break;
	default:
		break;
	}

	HeadingYaw += InputSteer * (Mode == EGCSurfMode::Surfboard ? 62.0f : 42.0f) * DeltaSeconds;
	const FRotator HeadingRot(0.0f, HeadingYaw, 0.0f);
	const FVector Forward = HeadingRot.Vector();
	const FVector Right = FRotationMatrix(HeadingRot).GetUnitAxis(EAxis::Y);

	FVector Accel = FVector::ZeroVector;
	// Gravity running down the wave face: the heart of surfing.
	const FVector SlopeAccel(-Sample.Normal.X, -Sample.Normal.Y, 0.0f);
	Accel += SlopeAccel * WaveSlopeAccel * (Mode == EGCSurfMode::Surfboard ? 1.25f : 0.75f);
	Accel += Forward * (InputThrottle * ModePaddle);
	Accel += Sample.Current * (Mode == EGCSurfMode::WhitewaterRaft ? 1.7f : 0.45f);
	if (Mode == EGCSurfMode::WhitewaterRaft && FoamFactor > 0.45f)
	{
		Accel += Sample.Current * 2.2f; // whitewater push on foamy crests
	}
	Accel += -PlanarVelocity * Drag;

	PlanarVelocity += Accel * DeltaSeconds;
	if (PlanarVelocity.Size() > ModeMaxSpeed)
	{
		PlanarVelocity = PlanarVelocity.GetSafeNormal() * ModeMaxSpeed;
	}
	SpeedKmh = PlanarVelocity.Size() * 0.036f;

	FVector NewLoc = Loc + PlanarVelocity * DeltaSeconds;
	NewLoc.Z = SurfaceZ + (Mode == EGCSurfMode::WhitewaterRaft ? 28.0f : 10.0f);

	if (IsBeached(NewLoc, SurfaceZ))
	{
		PlanarVelocity *= 0.25f;
		NewLoc = Loc;
		EndRide(false);
		return;
	}

	// Board pitch/roll follows the wave normal, lerped toward heading.
	FVector WaveUp = Sample.Normal;
	FVector WaveForward = FVector::VectorPlaneProject(Forward, WaveUp).GetSafeNormal();
	if (WaveForward.IsNearlyZero())
	{
		WaveForward = Forward;
	}
	const FMatrix Basis(WaveForward, FVector::CrossProduct(WaveUp, WaveForward).GetSafeNormal(), WaveUp, FVector::ZeroVector);
	FRotator TargetRot = Basis.Rotator();
	const float Lean = FMath::Clamp(InputSteer * -18.0f, -22.0f, 22.0f);
	TargetRot.Roll += Lean;
	SetActorLocationAndRotation(NewLoc, FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaSeconds, 6.0f));

	// Balance model (surf only): sustained over-lean + steep face = wipeout.
	const float RollStress = FMath::Abs(FVector::DotProduct(Sample.Normal, Right)) + FMath::Abs(InputSteer) * 0.32f;
	Balance = FMath::FInterpTo(Balance, (RollStress - 0.42f * Stability) * 2.0f, DeltaSeconds, 1.8f);
	if (Mode == EGCSurfMode::Surfboard && Balance > 0.75f)
	{
		BalanceTimeOverLimit += DeltaSeconds;
		if (BalanceTimeOverLimit > 0.65f)
		{
			EndRide(true);
			return;
		}
	}
	else
	{
		BalanceTimeOverLimit = FMath::Max(0.0f, BalanceTimeOverLimit - DeltaSeconds * 1.5f);
	}
}

void AGCSurfboardPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("VehicleThrottle"), this, &AGCSurfboardPawn::Input_Throttle);
	PlayerInputComponent->BindAxis(TEXT("VehicleSteer"), this, &AGCSurfboardPawn::Input_Steer);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AGCSurfboardPawn::Input_CameraTurn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AGCSurfboardPawn::Input_CameraLookUp);
	PlayerInputComponent->BindAction(TEXT("EnterVehicle"), IE_Pressed, this, &AGCSurfboardPawn::Input_Exit);
}

void AGCSurfboardPawn::Input_Throttle(float Value) { InputThrottle = FMath::Clamp(Value, -1.0f, 1.0f); }
void AGCSurfboardPawn::Input_Steer(float Value) { InputSteer = FMath::Clamp(Value, -1.0f, 1.0f); }
void AGCSurfboardPawn::Input_Exit() { EndRide(false); }
void AGCSurfboardPawn::Input_CameraTurn(float Value) { AddControllerYawInput(Value); }
void AGCSurfboardPawn::Input_CameraLookUp(float Value) { AddControllerPitchInput(Value); }
