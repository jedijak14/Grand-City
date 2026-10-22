#include "Vehicle/GCVehiclePawn.h"
#include "Character/GCCharacter.h"
#include "Components/GCDeformationComponent.h"
#include "Components/GCHealthComponent.h"
#include "Components/GCSkidmarkComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/SpringArmComponent.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AGCVehiclePawn::AGCVehiclePawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;

	CollisionBody = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBody"));
	RootComponent = CollisionBody;
	CollisionBody->SetBoxExtent(FVector(220.0f, 95.0f, 45.0f));
	CollisionBody->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionBody->SetSimulatePhysics(true);
	CollisionBody->SetEnableGravity(true);
	CollisionBody->SetLinearDamping(0.05f);
	CollisionBody->SetAngularDamping(0.45f);
	CollisionBody->SetNotifyRigidBodyCollision(true);
	CollisionBody->BodyInstance.bUseCCD = true;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(CollisionBody);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Most imported car models have their origin at ground level under the car centre.
	// Adjust this offset in your Blueprint child if your model differs.
	BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -65.0f));

	DeformedBodyMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("DeformedBodyMesh"));
	DeformedBodyMesh->SetupAttachment(CollisionBody);
	DeformedBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DeformedBodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -65.0f));

	Deformation = CreateDefaultSubobject<UGCDeformationComponent>(TEXT("Deformation"));
	Skidmarks = CreateDefaultSubobject<UGCSkidmarkComponent>(TEXT("Skidmarks"));
	Skidmarks->SetupAttachment(CollisionBody);

	Health = CreateDefaultSubobject<UGCHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 650.0f;

	DriverSeat = CreateDefaultSubobject<USceneComponent>(TEXT("DriverSeat"));
	DriverSeat->SetupAttachment(CollisionBody);
	DriverSeat->SetRelativeLocation(FVector(10.0f, -45.0f, 5.0f));

	ExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ExitPoint"));
	ExitPoint->SetupAttachment(CollisionBody);
	ExitPoint->SetRelativeLocation(FVector(10.0f, -190.0f, -20.0f));

	DriverDoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("DriverDoorPivot"));
	DriverDoorPivot->SetupAttachment(CollisionBody);
	DriverDoorPivot->SetRelativeLocation(FVector(85.0f, -95.0f, 0.0f));

	ChaseArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ChaseArm"));
	ChaseArm->SetupAttachment(CollisionBody);
	ChaseArm->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	ChaseArm->TargetArmLength = 680.0f;
	ChaseArm->bUsePawnControlRotation = true;
	ChaseArm->bEnableCameraLag = true;
	ChaseArm->CameraLagSpeed = 6.0f;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(ChaseArm);
	ChaseCamera->FieldOfView = 72.0f;

	CockpitCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CockpitCamera"));
	CockpitCamera->SetupAttachment(CollisionBody);
	CockpitCamera->SetRelativeLocation(FVector(35.0f, -45.0f, 38.0f));
	CockpitCamera->bUsePawnControlRotation = false;
	CockpitCamera->FieldOfView = 78.0f;
	CockpitCamera->SetActive(false);

	EngineAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));
	EngineAudio->SetupAttachment(CollisionBody);
	EngineAudio->bAutoActivate = false;

	ScreechAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("ScreechAudio"));
	ScreechAudio->SetupAttachment(CollisionBody);
	ScreechAudio->bAutoActivate = false;

	SirenAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("SirenAudio"));
	SirenAudio->SetupAttachment(CollisionBody);
	SirenAudio->bAutoActivate = false;

	HeadlightL = CreateDefaultSubobject<USpotLightComponent>(TEXT("HeadlightL"));
	HeadlightL->SetupAttachment(CollisionBody);
	HeadlightL->SetRelativeLocation(FVector(225.0f, -65.0f, 0.0f));
	HeadlightL->SetIntensity(0.0f);
	HeadlightL->SetAttenuationRadius(4500.0f);
	HeadlightL->SetOuterConeAngle(28.0f);

	HeadlightR = CreateDefaultSubobject<USpotLightComponent>(TEXT("HeadlightR"));
	HeadlightR->SetupAttachment(CollisionBody);
	HeadlightR->SetRelativeLocation(FVector(225.0f, 65.0f, 0.0f));
	HeadlightR->SetIntensity(0.0f);
	HeadlightR->SetAttenuationRadius(4500.0f);
	HeadlightR->SetOuterConeAngle(28.0f);

	SirenLightRed = CreateDefaultSubobject<UPointLightComponent>(TEXT("SirenLightRed"));
	SirenLightRed->SetupAttachment(CollisionBody);
	SirenLightRed->SetRelativeLocation(FVector(0.0f, -45.0f, 90.0f));
	SirenLightRed->SetLightColor(FColor::Red);
	SirenLightRed->SetIntensity(0.0f);

	SirenLightBlue = CreateDefaultSubobject<UPointLightComponent>(TEXT("SirenLightBlue"));
	SirenLightBlue->SetupAttachment(CollisionBody);
	SirenLightBlue->SetRelativeLocation(FVector(0.0f, 45.0f, 90.0f));
	SirenLightBlue->SetLightColor(FColor::Blue);
	SirenLightBlue->SetIntensity(0.0f);

	// Default wheel layout: FL, FR, RL, RR. Adjust per-car in Blueprint children.
	Wheels.Reset();
	FGCWheelSetup FL; FL.LocalOffset = FVector(145.0f, -82.0f, 5.0f); FL.bSteering = true;
	FGCWheelSetup FR; FR.LocalOffset = FVector(145.0f,  82.0f, 5.0f); FR.bSteering = true;
	FGCWheelSetup RL; RL.LocalOffset = FVector(-145.0f, -82.0f, 5.0f);
	FGCWheelSetup RR; RR.LocalOffset = FVector(-145.0f,  82.0f, 5.0f);
	Wheels = { FL, FR, RL, RR };

	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* WheelMesh = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("WheelMesh%d"), i));
		WheelMesh->SetupAttachment(CollisionBody);
		WheelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WheelMeshes.Add(WheelMesh);
	}

	AutoPossessAI = EAutoPossessAI::Disabled;
}

void AGCVehiclePawn::BeginPlay()
{
	Super::BeginPlay();

	WheelState.SetNum(Wheels.Num());
	for (int32 i = 0; i < WheelState.Num(); ++i)
	{
		WheelState[i].bDrive = IsDriveWheel(i);
	}

	CollisionBody->SetMassOverrideInKg(NAME_None, MassKg, true);
	CollisionBody->RecreatePhysicsState();
	CollisionBody->OnComponentHit.AddDynamic(this, &AGCVehiclePawn::OnBodyHit);

	if (Health)
	{
		Health->OnDeath.AddDynamic(this, &AGCVehiclePawn::OnVehicleDeath);
	}

	// Swap the visual body into a deformable procedural mesh (no-op if mesh lacks CPU access).
	if (Deformation && BodyMesh && BodyMesh->GetStaticMesh())
	{
		DeformedBodyMesh->SetRelativeTransform(BodyMesh->GetRelativeTransform());
		if (!Deformation->InitializeFromStaticMesh(BodyMesh, DeformedBodyMesh))
		{
			DeformedBodyMesh->SetVisibility(false);
		}
	}
	else
	{
		DeformedBodyMesh->SetVisibility(false);
	}

	SetHeadlights(false);
	SetSiren(bIsPoliceVehicle ? false : false);
}

void AGCVehiclePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateVehiclePhysics(DeltaSeconds);
	UpdateWheelVisuals(DeltaSeconds);
	UpdateOccupancy(DeltaSeconds);
	UpdateAudioAndLights(DeltaSeconds);

	// Speed-sensitive FOV kick in chase cam.
	if (ChaseCamera)
	{
		const float TargetFOV = 72.0f + FMath::Clamp(GetSpeedKmh() / 220.0f, 0.0f, 1.0f) * 14.0f;
		ChaseCamera->FieldOfView = FMath::FInterpTo(ChaseCamera->FieldOfView, TargetFOV, DeltaSeconds, 2.5f);
	}
}

// ------------------------------------------------------------------ physics
FVector AGCVehiclePawn::GetPointVelocity(const FVector& WorldPoint) const
{
	const FVector Linear = CollisionBody->GetPhysicsLinearVelocity();
	const FVector Angular = CollisionBody->GetPhysicsAngularVelocityInRadians();
	return Linear + FVector::CrossProduct(Angular, WorldPoint - CollisionBody->GetCenterOfMass());
}

bool AGCVehiclePawn::IsDriveWheel(int32 Index) const
{
	if (!Wheels.IsValidIndex(Index))
	{
		return false;
	}
	const bool bFront = Wheels[Index].LocalOffset.X > 0.0f;
	switch (DriveType)
	{
	case EGCVehicleDriveType::FWD: return bFront;
	case EGCVehicleDriveType::AWD: return true;
	default: return !bFront;
	}
}

float AGCVehiclePawn::GetMaxSteerRad() const
{
	const float SpeedT = FMath::Clamp(FMath::Abs(GetSpeedKmh()) / 180.0f, 0.0f, 1.0f);
	const float Deg = FMath::Lerp(MaxSteerAngleDegrees, HighSpeedSteerAngleDegrees, SpeedT);
	return FMath::DegreesToRadians(Deg);
}

void AGCVehiclePawn::UpdatePowertrain(float DeltaSeconds, float& OutTotalDriveForceN)
{
	OutTotalDriveForceN = 0.0f;
	if (bWrecked)
	{
		CurrentRPM = FMath::FInterpTo(CurrentRPM, 0.0f, DeltaSeconds, 2.0f);
		return;
	}

	const float ForwardSpeed = GetForwardSpeedCms(); // cm/s
	const float Throttle = FMath::Clamp(InputThrottle, -1.0f, 1.0f);

	// Decide gear: pressing "back" at speed = brake (handled in wheel forces), at rest = reverse.
	if (Throttle < -0.05f && ForwardSpeed < 120.0f)
	{
		CurrentGear = -1;
	}
	else if (CurrentGear <= 0 && Throttle > 0.05f)
	{
		CurrentGear = 1;
	}
	else if (CurrentGear == 0)
	{
		CurrentGear = 1;
	}

	// Engine RPM from average drive-wheel speed.
	float AvgDriveSpeedMps = 0.0f;
	int32 DriveCount = 0;
	float AvgRadiusM = 0.34f;
	for (int32 i = 0; i < WheelState.Num(); ++i)
	{
		if (WheelState[i].bDrive)
		{
			AvgDriveSpeedMps += FMath::Abs(WheelState[i].LongitudinalSpeedMps);
			AvgRadiusM = Wheels[i].Radius / 100.0f;
			++DriveCount;
		}
	}
	if (DriveCount > 0)
	{
		AvgDriveSpeedMps /= DriveCount;
	}
	const float GearRatio = CurrentGear < 0 ? ReverseGearRatio
		: (GearRatios.IsValidIndex(CurrentGear - 1) ? GearRatios[CurrentGear - 1] : 1.0f);
	const float WheelRPM = (AvgDriveSpeedMps / FMath::Max(0.05f, 2.0f * PI * AvgRadiusM)) * 60.0f;
	float TargetRPM = FMath::Clamp(WheelRPM * GearRatio * FinalDriveRatio, IdleRPM, MaxRPM);
	if (FMath::Abs(Throttle) > 0.05f && AvgDriveSpeedMps < 1.0f)
	{
		TargetRPM = FMath::Max(TargetRPM, IdleRPM + FMath::Abs(Throttle) * 2200.0f); // revving at launch
	}
	CurrentRPM = FMath::FInterpTo(CurrentRPM, TargetRPM, DeltaSeconds, 6.0f);

	// Automatic shifts.
	ShiftTimer = FMath::Max(0.0f, ShiftTimer - DeltaSeconds);
	if (CurrentGear > 0 && ShiftTimer <= 0.0f)
	{
		if (CurrentRPM > 5800.0f && CurrentGear < GearRatios.Num())
		{
			++CurrentGear;
			ShiftTimer = 0.35f;
		}
		else if (CurrentRPM < 2100.0f && CurrentGear > 1)
		{
			--CurrentGear;
			ShiftTimer = 0.25f;
		}
	}

	// Simple torque-curve shape: peak around 55% of rev range, torque cut during a shift.
	const float Norm = FMath::Clamp((CurrentRPM - IdleRPM) / FMath::Max(1.0f, MaxRPM - IdleRPM), 0.0f, 1.0f);
	float TorqueFactor = FMath::Clamp(1.15f - FMath::Abs(Norm - 0.55f) * 0.9f, 0.35f, 1.0f);
	if (ShiftTimer > 0.0f)
	{
		TorqueFactor *= 0.3f;
	}

	const float Ratio = CurrentGear < 0 ? ReverseGearRatio
		: (GearRatios.IsValidIndex(CurrentGear - 1) ? GearRatios[CurrentGear - 1] : 1.0f);
	OutTotalDriveForceN = Throttle * EngineMaxTorqueNm * TorqueFactor * Ratio * FinalDriveRatio
		/ FMath::Max(0.1f, AvgRadiusM);
}

void AGCVehiclePawn::UpdateVehiclePhysics(float DeltaSeconds)
{
	if (!CollisionBody->IsSimulatingPhysics() || DeltaSeconds <= 0.0f)
	{
		return;
	}
	const int32 Substeps = FMath::Clamp(FMath::CeilToInt(DeltaSeconds / 0.008f), 1, 4);
	const float SubDt = DeltaSeconds / Substeps;
	for (int32 s = 0; s < Substeps; ++s)
	{
		ApplyWheelForces(SubDt);
	}
}

void AGCVehiclePawn::ApplyWheelForces(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	float TotalDriveForceN = 0.0f;
	UpdatePowertrain(DeltaSeconds, TotalDriveForceN);

	const FTransform BodyTM = CollisionBody->GetComponentTransform();
	const FVector Up = BodyTM.GetUnitAxis(EAxis::Z);
	const FVector Forward = BodyTM.GetUnitAxis(EAxis::X);
	const FVector Right = BodyTM.GetUnitAxis(EAxis::Y);
	const FVector Velocity = CollisionBody->GetPhysicsLinearVelocity();
	const float ForwardSpeed = FVector::DotProduct(Velocity, Forward);

	// Brake vs reverse from the same pedals, GTA-style.
	float BrakeInput = 0.0f;
	if (InputThrottle < -0.05f && ForwardSpeed > 120.0f)
	{
		BrakeInput = -InputThrottle;
	}
	if (VehicleState == EGCVehicleState::Empty && FMath::Abs(InputThrottle) < 0.02f)
	{
		BrakeInput = 1.0f; // parking brake when nobody is driving
	}

	// Steering (smoothed, speed sensitive).
	const float TargetSteer = FMath::Clamp(InputSteering, -1.0f, 1.0f) * GetMaxSteerRad();
	CurrentSteerAngleRad = FMath::FInterpTo(CurrentSteerAngleRad, TargetSteer, DeltaSeconds, SteerSpeed);

	int32 DriveWheelCount = 0;
	for (const FWheelRuntime& WS : WheelState)
	{
		if (WS.bDrive) { ++DriveWheelCount; }
	}
	const float DriveForcePerWheel = DriveWheelCount > 0 ? TotalDriveForceN / DriveWheelCount : 0.0f;
	const float BrakeForcePerWheel = Wheels.Num() > 0 ? (BrakeInput * MaxBrakeForceN) / Wheels.Num() : 0.0f;

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(SuspensionTrace), false, this);
	if (DriverCharacter)
	{
		TraceParams.AddIgnoredActor(DriverCharacter);
	}

	bDrifting = false;

	// Pass 1: raycasts + suspension.
	for (int32 i = 0; i < Wheels.Num(); ++i)
	{
		const FGCWheelSetup& Setup = Wheels[i];
		FWheelRuntime& WS = WheelState[i];

		// Ackermann: inner wheel steers more than the outer wheel.
		float SteerRad = 0.0f;
		if (Setup.bSteering)
		{
			SteerRad = CurrentSteerAngleRad;
			if (!FMath::IsNearlyZero(SteerRad, 0.001f) && Wheels.Num() >= 4)
			{
				const float Wheelbase = FMath::Abs(Wheels[0].LocalOffset.X - Wheels[2].LocalOffset.X);
				const float Track = FMath::Abs(Wheels[0].LocalOffset.Y - Wheels[1].LocalOffset.Y);
				const float TurnRadius = Wheelbase / FMath::Tan(SteerRad);
				const float Side = Setup.LocalOffset.Y > 0.0f ? 1.0f : -1.0f;
				const float Adj = SteerRad > 0.0f ? (Side * Track * 0.5f) : (-Side * Track * 0.5f);
				SteerRad = FMath::Atan(Wheelbase / FMath::Max(50.0f, FMath::Abs(TurnRadius) + Adj)) * FMath::Sign(SteerRad);
			}
		}
		WS.SteerAngleRad = SteerRad;

		const FVector Anchor = BodyTM.TransformPosition(Setup.LocalOffset);
		const FVector TraceEnd = Anchor - Up * (SuspensionRestLength + Setup.Radius);
		FHitResult Hit;
		WS.bGrounded = World->LineTraceSingleByChannel(Hit, Anchor, TraceEnd, ECC_Visibility, TraceParams);

		if (WS.bGrounded)
		{
			const float SuspLen = FMath::Clamp(Hit.Distance - Setup.Radius, 0.0f, SuspensionRestLength);
			const float CompressionCm = SuspensionRestLength - SuspLen;
			WS.Compression = CompressionCm / 100.0f;

			const FVector AnchorVel = GetPointVelocity(Anchor);
			const float CompressionSpeedMps = FVector::DotProduct(AnchorVel, -Up) / 100.0f;
			WS.SuspensionForceN = FMath::Max(0.0f,
				SpringRateNPerM * WS.Compression + DampingNPerMS * CompressionSpeedMps);
			WS.ContactPoint = Hit.ImpactPoint;
			WS.ContactNormal = Hit.ImpactNormal;
		}
		else
		{
			WS.Compression = 0.0f;
			WS.SuspensionForceN = 0.0f;
			WS.ContactPoint = Anchor - Up * SuspensionRestLength;
			WS.ContactNormal = Up;
		}

		if (bDebugDrawWheels)
		{
			DrawDebugLine(World, Anchor, TraceEnd, WS.bGrounded ? FColor::Green : FColor::Red, false, -1.0f, 0, 1.5f);
		}
	}

	// Anti-roll: push load from the compressed side to the extended side per axle (FL/FR, RL/RR).
	if (WheelState.Num() >= 4)
	{
		for (int32 Axle = 0; Axle < 2; ++Axle)
		{
			FWheelRuntime& L = WheelState[Axle * 2 + 0];
			FWheelRuntime& R = WheelState[Axle * 2 + 1];
			if (L.bGrounded && R.bGrounded)
			{
				const float Diff = (L.Compression - R.Compression) * AntiRollStiffness;
				L.SuspensionForceN = FMath::Max(0.0f, L.SuspensionForceN + Diff);
				R.SuspensionForceN = FMath::Max(0.0f, R.SuspensionForceN - Diff);
			}
		}
	}

	// Pass 2: apply suspension + tyre forces.
	for (int32 i = 0; i < Wheels.Num(); ++i)
	{
		const FGCWheelSetup& Setup = Wheels[i];
		FWheelRuntime& WS = WheelState[i];
		WS.bSlipping = false;
		if (!WS.bGrounded)
		{
			WS.LateralSlipMps = 0.0f;
			WS.LongitudinalSpeedMps = 0.0f;
			Skidmarks->EndSkid(i);
			continue;
		}

		const FVector Anchor = BodyTM.TransformPosition(Setup.LocalOffset);
		// Suspension pushes the body up at the anchor.
		CollisionBody->AddForceAtLocation(Up * WS.SuspensionForceN * 100.0f, Anchor);

		const FVector WheelForward = Forward.RotateAngleAxisRad(WS.SteerAngleRad, Up);
		const FVector WheelRight = Right.RotateAngleAxisRad(WS.SteerAngleRad, Up);
		const FVector ContactVel = GetPointVelocity(WS.ContactPoint);
		const float LongVel = FVector::DotProduct(ContactVel, WheelForward) / 100.0f; // m/s
		const float LatVel = FVector::DotProduct(ContactVel, WheelRight) / 100.0f;    // m/s
		WS.LongitudinalSpeedMps = LongVel;
		WS.LateralSlipMps = LatVel;

		const bool bRear = Setup.LocalOffset.X < 0.0f;
		float MuLat = TireFrictionMu;
		if (bInputHandbrake && bRear)
		{
			MuLat *= HandbrakeRearGripMultiplier;
		}

		// Lateral tyre force saturates with slip velocity (tanh Pacejka-lite).
		const float MaxLatForce = MuLat * WS.SuspensionForceN;
		const float LatForce = -MaxLatForce * FMath::Tanh(LatVel * LateralGripStiffness);
		CollisionBody->AddForceAtLocation(WheelRight * LatForce * 100.0f, WS.ContactPoint);

		// Friction circle: whatever grip the lateral force didn't use is available longitudinally.
		const float MaxTotal = TireFrictionMu * WS.SuspensionForceN;
		const float AvailableLong = FMath::Sqrt(FMath::Max(0.0f, MaxTotal * MaxTotal - LatForce * LatForce));

		float LongForce = 0.0f;
		if (WS.bDrive)
		{
			LongForce += DriveForcePerWheel;
			if (FMath::Abs(DriveForcePerWheel) > AvailableLong + 50.0f && FMath::Abs(InputThrottle) > 0.3f)
			{
				WS.bSlipping = true; // wheelspin
			}
		}
		// Brakes (+ handbrake on the rear axle).
		float BrakeForce = BrakeForcePerWheel;
		if (bInputHandbrake && bRear)
		{
			BrakeForce += MaxBrakeForceN * 0.35f;
		}
		if (BrakeForce > 0.0f && FMath::Abs(LongVel) > 0.15f)
		{
			LongForce += -FMath::Sign(LongVel) * BrakeForce;
		}
		// Rolling resistance.
		LongForce += -LongVel * RollingResistanceCoeff * (WS.SuspensionForceN / 4000.0f);

		LongForce = FMath::Clamp(LongForce, -AvailableLong, AvailableLong);
		CollisionBody->AddForceAtLocation(WheelForward * LongForce * 100.0f, WS.ContactPoint);

		if (FMath::Abs(LatVel) > 2.2f)
		{
			WS.bSlipping = true;
		}
		if (bRear && FMath::Abs(LatVel) > 3.0f && FMath::Abs(LongVel) > 8.0f)
		{
			bDrifting = true;
		}

		if (WS.bSlipping && FMath::Abs(LongVel) > 2.5f)
		{
			const float Alpha = FMath::Clamp(FMath::Abs(LatVel) / 9.0f + (WS.bDrive ? 0.25f : 0.0f), 0.15f, 0.9f);
			Skidmarks->AddSkidPoint(i, WS.ContactPoint, WS.ContactNormal, Setup.Width, Alpha);
		}
		else
		{
			Skidmarks->EndSkid(i);
		}
	}

	// Aero drag + downforce at the centre of mass.
	const float SpeedMps = Velocity.Size() / 100.0f;
	if (SpeedMps > 0.5f)
	{
		const FVector DragForce = -Velocity.GetSafeNormal() * (DragCoeff * SpeedMps * SpeedMps * 100.0f);
		CollisionBody->AddForceAtLocation(DragForce, CollisionBody->GetCenterOfMass());
		CollisionBody->AddForceAtLocation(-Up * (DownforceCoeff * SpeedMps * SpeedMps * 100.0f),
			CollisionBody->GetCenterOfMass());
	}
}

void AGCVehiclePawn::UpdateWheelVisuals(float DeltaSeconds)
{
	for (int32 i = 0; i < Wheels.Num() && i < WheelMeshes.Num(); ++i)
	{
		UStaticMeshComponent* Mesh = WheelMeshes[i];
		if (!Mesh)
		{
			continue;
		}
		FWheelRuntime& WS = WheelState[i];
		const float SuspLen = WS.bGrounded ? (SuspensionRestLength - WS.Compression * 100.0f) : SuspensionRestLength;
		const FVector LocalPos = Wheels[i].LocalOffset + FVector(0.0f, 0.0f, -SuspLen);
		WS.SpinAngleDeg += (WS.LongitudinalSpeedMps * 100.0f / FMath::Max(5.0f, Wheels[i].Radius)) * DeltaSeconds * (180.0f / PI);
		WS.SpinAngleDeg = FMath::Fmod(WS.SpinAngleDeg, 360.0f);
		Mesh->SetRelativeLocation(LocalPos);
		// UE car convention: forward X, steer = yaw (Z), spin = pitch (Y).
		Mesh->SetRelativeRotation(FRotator(WS.SpinAngleDeg, FMath::RadiansToDegrees(WS.SteerAngleRad), WheelVisualRollOffsetDeg));
	}
}

void AGCVehiclePawn::UpdateAudioAndLights(float DeltaSeconds)
{
	if (EngineAudio)
	{
		const bool bEngineShouldRun = !bWrecked && (VehicleState == EGCVehicleState::Occupied || FMath::Abs(InputThrottle) > 0.02f);
		if (bEngineShouldRun && EngineAudio->GetSound() && !EngineAudio->IsPlaying())
		{
			EngineAudio->Play();
		}
		if (EngineAudio->IsPlaying())
		{
			EngineAudio->SetPitchMultiplier(0.7f + (CurrentRPM / FMath::Max(1.0f, MaxRPM)) * 1.7f);
			EngineAudio->SetVolumeMultiplier(0.55f + FMath::Abs(InputThrottle) * 0.45f);
			if (!bEngineShouldRun)
			{
				EngineAudio->Stop();
			}
		}
	}

	if (ScreechAudio)
	{
		bool bAnySlip = false;
		for (const FWheelRuntime& WS : WheelState)
		{
			bAnySlip |= (WS.bGrounded && WS.bSlipping);
		}
		if (bAnySlip && ScreechAudio->GetSound() && !ScreechAudio->IsPlaying())
		{
			ScreechAudio->Play();
		}
		else if (!bAnySlip && ScreechAudio->IsPlaying())
		{
			ScreechAudio->Stop();
		}
		if (ScreechAudio->IsPlaying())
		{
			ScreechAudio->SetVolumeMultiplier(bDrifting ? 1.0f : 0.65f);
		}
	}

	if (bSirenOn)
	{
		if (SirenAudio && SirenAudio->GetSound() && !SirenAudio->IsPlaying())
		{
			SirenAudio->Play();
		}
		SirenFlashTimer += DeltaSeconds;
		if (SirenFlashTimer > 0.42f)
		{
			SirenFlashTimer = 0.0f;
			bSirenPhase = !bSirenPhase;
		}
		SirenLightRed->SetIntensity(bSirenPhase ? 6000.0f : 0.0f);
		SirenLightBlue->SetIntensity(bSirenPhase ? 0.0f : 6000.0f);
	}
	else
	{
		if (SirenAudio && SirenAudio->IsPlaying())
		{
			SirenAudio->Stop();
		}
		SirenLightRed->SetIntensity(0.0f);
		SirenLightBlue->SetIntensity(0.0f);
	}
}

// ------------------------------------------------------------------ occupancy
void AGCVehiclePawn::SetDoorAngle(float Alpha)
{
	DoorOpenAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	if (DriverDoorPivot)
	{
		// Driver sits on -Y side; door swings outward.
		DriverDoorPivot->SetRelativeRotation(FRotator(0.0f, -72.0f * DoorOpenAlpha, 0.0f));
	}
}

bool AGCVehiclePawn::BeginEnter(AGCCharacter* Character)
{
	if (!Character || VehicleState != EGCVehicleState::Empty)
	{
		return false;
	}
	if (FVector::Distance(GetActorLocation(), Character->GetActorLocation()) > 450.0f)
	{
		return false;
	}
	DriverCharacter = Character;
	VehicleState = EGCVehicleState::Entering;
	OccupancyTimer = 0.0f;
	EnterStartLocation = Character->GetActorLocation();
	EnterStartRotation = Character->GetActorRotation();
	Character->BeginVehicleTransition();
	return true;
}

void AGCVehiclePawn::BeginExit()
{
	if (VehicleState != EGCVehicleState::Occupied || !DriverCharacter)
	{
		return;
	}
	VehicleState = EGCVehicleState::Exiting;
	OccupancyTimer = 0.0f;
	EnterStartLocation = DriverSeat->GetComponentLocation();
	EnterStartRotation = DriverCharacter->GetActorRotation();
	DriverCharacter->BeginVehicleTransition();
	InputThrottle = 0.0f;
	InputSteering = 0.0f;
	bInputHandbrake = true;
}

void AGCVehiclePawn::UpdateOccupancy(float DeltaSeconds)
{
	if (VehicleState == EGCVehicleState::Entering && DriverCharacter)
	{
		OccupancyTimer += DeltaSeconds;
		const float Duration = 0.9f;
		const float T = FMath::Clamp(OccupancyTimer / Duration, 0.0f, 1.0f);
		SetDoorAngle(T < 0.45f ? (T / 0.45f) : (T > 0.6f ? (1.0f - (T - 0.6f) / 0.4f) : 1.0f));

		const FVector SeatLoc = DriverSeat->GetComponentLocation();
		const FRotator SeatRot = DriverSeat->GetComponentRotation();
		DriverCharacter->SetActorLocation(FMath::Lerp(EnterStartLocation, SeatLoc, T));
		DriverCharacter->SetActorRotation(FMath::RInterpTo(EnterStartRotation, SeatRot, 1.0f, T));

		if (T >= 1.0f)
		{
			VehicleState = EGCVehicleState::Occupied;
			SetDoorAngle(0.0f);
			AController* CharController = DriverCharacter->GetController();
			DriverCharacter->CompleteEnterVehicle(this, DriverSeat);
			if (CharController)
			{
				CharController->Possess(this);
			}
			bInputHandbrake = false;
		}
	}
	else if (VehicleState == EGCVehicleState::Exiting && DriverCharacter)
	{
		OccupancyTimer += DeltaSeconds;
		const float Duration = 0.75f;
		const float T = FMath::Clamp(OccupancyTimer / Duration, 0.0f, 1.0f);
		SetDoorAngle(T < 0.4f ? (T / 0.4f) : (T > 0.65f ? (1.0f - (T - 0.65f) / 0.35f) : 1.0f));

		const FVector ExitLoc = ExitPoint->GetComponentLocation();
		DriverCharacter->SetActorLocation(FMath::Lerp(EnterStartLocation, ExitLoc, T));

		if (T >= 1.0f)
		{
			SetDoorAngle(0.0f);
			AController* VehicleController = GetController();
			// Drop the character to the ground beside the car.
			FVector GroundLoc = ExitLoc;
			FHitResult GroundHit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(ExitGround), false, this);
			Params.AddIgnoredActor(DriverCharacter);
			if (GetWorld()->LineTraceSingleByChannel(GroundHit, ExitLoc + FVector(0, 0, 200.0f),
				ExitLoc - FVector(0, 0, 400.0f), ECC_Visibility, Params))
			{
				GroundLoc = GroundHit.ImpactPoint + FVector(0, 0, 92.0f);
			}
			DriverCharacter->CompleteExitVehicle(GroundLoc);
			if (VehicleController)
			{
				VehicleController->Possess(DriverCharacter);
			}
			DriverCharacter = nullptr;
			VehicleState = EGCVehicleState::Empty;
		}
	}
}

// ------------------------------------------------------------------ crash
void AGCVehiclePawn::OnBodyHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this || OtherActor == DriverCharacter)
	{
		return;
	}
	const FVector PointVel = GetPointVelocity(Hit.ImpactPoint);
	const float ClosingSpeed = -FVector::DotProduct(PointVel, Hit.ImpactNormal); // cm/s into the surface
	if (ClosingSpeed < MinCrashSpeedForDent)
	{
		return;
	}

	const float Severity = FMath::Clamp((ClosingSpeed - MinCrashSpeedForDent) / 1800.0f, 0.05f, 1.0f);
	if (Deformation)
	{
		const float Radius = FMath::Lerp(55.0f, 170.0f, Severity);
		const float Depth = FMath::Lerp(4.0f, 36.0f, Severity);
		Deformation->ApplyDentWorld(Hit.ImpactPoint, -Hit.ImpactNormal, Radius, Depth);
	}

	TakeDamage(Severity * 120.0f, FDamageEvent(), nullptr, OtherActor);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (CrashShakeClass)
		{
			PC->ClientStartCameraShake(CrashShakeClass, 0.4f + Severity);
		}
	}
}

void AGCVehiclePawn::OnVehicleDeath(AActor* Killer)
{
	bWrecked = true;
	InputThrottle = 0.0f;
	if (EngineAudio)
	{
		EngineAudio->Stop();
	}
	// Smoke/fire Niagara hooks go here in a Blueprint child via Health->OnDeath if desired.
}

// ------------------------------------------------------------------ misc API
float AGCVehiclePawn::GetForwardSpeedCms() const
{
	return FVector::DotProduct(CollisionBody->GetPhysicsLinearVelocity(), GetActorForwardVector());
}

float AGCVehiclePawn::GetSpeedKmh() const
{
	return CollisionBody->GetPhysicsLinearVelocity().Size() * 0.036f;
}

float AGCVehiclePawn::GetSteeringNormalized() const
{
	const float MaxSteer = GetMaxSteerRad();
	return MaxSteer > 0.001f ? FMath::Clamp(CurrentSteerAngleRad / MaxSteer, -1.0f, 1.0f) : 0.0f;
}

void AGCVehiclePawn::SetAIInputs(float Throttle, float Steering, bool bInHandbrake)
{
	InputThrottle = FMath::Clamp(Throttle, -1.0f, 1.0f);
	InputSteering = FMath::Clamp(Steering, -1.0f, 1.0f);
	bInputHandbrake = bInHandbrake;
}

void AGCVehiclePawn::SetSiren(bool bOn)
{
	bSirenOn = bOn && bIsPoliceVehicle;
}

void AGCVehiclePawn::SetHeadlights(bool bOn)
{
	bHeadlightsOn = bOn;
	const float I = bOn ? 9000.0f : 0.0f;
	if (HeadlightL) { HeadlightL->SetIntensity(I); }
	if (HeadlightR) { HeadlightR->SetIntensity(I); }
}

// ------------------------------------------------------------------ player input
void AGCVehiclePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("VehicleThrottle"), this, &AGCVehiclePawn::Input_Throttle);
	PlayerInputComponent->BindAxis(TEXT("VehicleSteer"), this, &AGCVehiclePawn::Input_Steer);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AGCVehiclePawn::Input_CameraTurn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AGCVehiclePawn::Input_CameraLookUp);
	PlayerInputComponent->BindAction(TEXT("Handbrake"), IE_Pressed, this, &AGCVehiclePawn::Input_HandbrakePressed);
	PlayerInputComponent->BindAction(TEXT("Handbrake"), IE_Released, this, &AGCVehiclePawn::Input_HandbrakeReleased);
	PlayerInputComponent->BindAction(TEXT("EnterVehicle"), IE_Pressed, this, &AGCVehiclePawn::Input_ExitPressed);
	PlayerInputComponent->BindAction(TEXT("ToggleCamera"), IE_Pressed, this, &AGCVehiclePawn::Input_ToggleCamera);
	PlayerInputComponent->BindAction(TEXT("VehicleLights"), IE_Pressed, this, &AGCVehiclePawn::Input_ToggleLights);
	PlayerInputComponent->BindAction(TEXT("ToggleSiren"), IE_Pressed, this, &AGCVehiclePawn::Input_ToggleSiren);
}

void AGCVehiclePawn::Input_Throttle(float Value) { InputThrottle = FMath::Clamp(Value, -1.0f, 1.0f); }
void AGCVehiclePawn::Input_Steer(float Value) { InputSteering = FMath::Clamp(Value, -1.0f, 1.0f); }
void AGCVehiclePawn::Input_HandbrakePressed() { bInputHandbrake = true; }
void AGCVehiclePawn::Input_HandbrakeReleased() { bInputHandbrake = false; }
void AGCVehiclePawn::Input_ExitPressed() { BeginExit(); }
void AGCVehiclePawn::Input_ToggleLights() { SetHeadlights(!bHeadlightsOn); }
void AGCVehiclePawn::Input_ToggleSiren() { SetSiren(!bSirenOn); }
void AGCVehiclePawn::Input_CameraTurn(float Value) { AddControllerYawInput(Value); }
void AGCVehiclePawn::Input_CameraLookUp(float Value) { AddControllerPitchInput(Value); }

void AGCVehiclePawn::Input_ToggleCamera()
{
	CameraMode = (CameraMode == EGCCameraMode::Chase) ? EGCCameraMode::Cockpit : EGCCameraMode::Chase;
	const bool bCockpit = CameraMode == EGCCameraMode::Cockpit;
	CockpitCamera->SetActive(bCockpit);
	ChaseCamera->SetActive(!bCockpit);
	if (DriverCharacter)
	{
		DriverCharacter->SetFirstPersonHidden(bCockpit);
	}
}
