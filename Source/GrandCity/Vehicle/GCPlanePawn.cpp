#include "Vehicle/GCPlanePawn.h"
#include "Camera/CameraComponent.h"
#include "Character/GCCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "InputCoreTypes.h"
#include "Components/GCHealthComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/SpringArmComponent.h"
#include "World/GCOceanManager.h"

AGCPlanePawn::AGCPlanePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
	RootComponent = Body;
	Body->SetBoxExtent(FVector(420.0f, 520.0f, 110.0f)); // fuselage + wingspan
	Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Body->SetSimulatePhysics(true);
	Body->SetLinearDamping(0.02f);
	Body->SetAngularDamping(0.65f);
	Body->BodyInstance.bUseCCD = true;

	FuselageMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FuselageMesh"));
	FuselageMesh->SetupAttachment(Body);
	FuselageMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FuselageMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -110.0f));

	PropellerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PropellerMesh"));
	PropellerMesh->SetupAttachment(Body);
	PropellerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PropellerMesh->SetRelativeLocation(FVector(430.0f, 0.0f, 0.0f));

	PilotSeat = CreateDefaultSubobject<USceneComponent>(TEXT("PilotSeat"));
	PilotSeat->SetupAttachment(Body);
	PilotSeat->SetRelativeLocation(FVector(60.0f, -35.0f, 30.0f));

	ExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ExitPoint"));
	ExitPoint->SetupAttachment(Body);
	ExitPoint->SetRelativeLocation(FVector(60.0f, -260.0f, -40.0f));

	ChaseArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ChaseArm"));
	ChaseArm->SetupAttachment(Body);
	ChaseArm->SetRelativeLocation(FVector(-100.0f, 0.0f, 180.0f));
	ChaseArm->TargetArmLength = 1300.0f;
	ChaseArm->bUsePawnControlRotation = false;
	ChaseArm->bInheritPitch = false;
	ChaseArm->bInheritRoll = false;
	ChaseArm->bEnableCameraLag = true;
	ChaseArm->CameraLagSpeed = 3.5f;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(ChaseArm);
	ChaseCamera->FieldOfView = 75.0f;

	Health = CreateDefaultSubobject<UGCHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 800.0f;

	EngineAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));
	EngineAudio->SetupAttachment(Body);
	EngineAudio->bAutoActivate = false;
}

void AGCPlanePawn::BeginPlay()
{
	Super::BeginPlay();
	Body->SetMassOverrideInKg(NAME_None, MassKg, true);
	Body->RecreatePhysicsState();

	float NearestDistanceSq = TNumericLimits<float>::Max();
	for (TActorIterator<AGCOceanManager> It(GetWorld()); It; ++It)
	{
		const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), It->GetActorLocation());
		if (DistanceSq < NearestDistanceSq)
		{
			NearestDistanceSq = DistanceSq;
			OceanManager = *It;
		}
	}
}

void AGCPlanePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bOccupied)
	{
		Throttle = FMath::Clamp(Throttle + ThrottleDeltaInput * DeltaSeconds * 0.55f, 0.0f, 1.0f);
	}
	if (FuelPercent > 0.0f)
	{
		FuelPercent = FMath::Max(0.0f, FuelPercent - Throttle * FuelBurnPercentPerSecond * DeltaSeconds);
	}
	ApplyFlightPhysics(DeltaSeconds);

	// A fast aircraft only disturbs the ocean during a genuinely low pass over the water surface.
	WaterWakeTimer -= DeltaSeconds;
	if (OceanManager && WaterWakeTimer <= 0.0f)
	{
		const FVector Location = GetActorLocation();
		const FVector Velocity = Body->GetPhysicsLinearVelocity();
		const float SpeedMps = Velocity.Size() / 100.0f;
		const FVector2D HalfExtent = OceanManager->VisualSurfaceHalfExtent;
		const FVector OceanLocation = OceanManager->GetActorLocation();
		const bool bOverOcean = !OceanManager->bGenerateVisualSurface
			|| (FMath::Abs(Location.X - OceanLocation.X) <= HalfExtent.X
				&& FMath::Abs(Location.Y - OceanLocation.Y) <= HalfExtent.Y);
		const float HeightAboveWater = Location.Z - OceanManager->SampleOcean(Location).Height;
		if (bOverOcean && HeightAboveWater >= -100.0f && HeightAboveWater <= 2500.0f && SpeedMps >= 40.0f)
		{
			const float HeightFactor = 1.0f - HeightAboveWater / 2500.0f;
			const float WakeStrengthCm = FMath::Clamp((SpeedMps - 40.0f) * 0.14f * HeightFactor, 0.5f, 12.0f);
			OceanManager->EmitWake(Location - GetActorForwardVector() * 450.0f,
				GetActorForwardVector(), WakeStrengthCm, 3.0f, 2200.0f, 2800.0f);
			WaterWakeTimer = 0.14f;
		}
		else
		{
			WaterWakeTimer = 0.35f;
		}
	}

	if (EngineAudio && EngineAudio->GetSound())
	{
		const bool bEngineOn = bOccupied && FuelPercent > 0.0f && Throttle > 0.02f;
		if (bEngineOn && !EngineAudio->IsPlaying())
		{
			EngineAudio->Play();
		}
		if (EngineAudio->IsPlaying())
		{
			EngineAudio->SetPitchMultiplier(0.65f + Throttle * 1.15f);
			EngineAudio->SetVolumeMultiplier(0.5f + Throttle * 0.5f);
			if (!bEngineOn)
			{
				EngineAudio->Stop();
			}
		}
	}

	const float RotorSpinRate = bHelicopterMode ? 900.0f + Throttle * 5400.0f : 120.0f + Throttle * 2600.0f;
	PropSpinDeg = FMath::Fmod(PropSpinDeg + RotorSpinRate * DeltaSeconds, 360.0f);
	if (PropellerMesh)
	{
		PropellerMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, PropSpinDeg));
	}
}

void AGCPlanePawn::ApplyFlightPhysics(float DeltaSeconds)
{
	if (!Body->IsSimulatingPhysics())
	{
		return;
	}

	const FVector Vel = Body->GetPhysicsLinearVelocity(); // cm/s
	const float SpeedMps = Vel.Size() / 100.0f;
	const FVector Forward = GetActorForwardVector();
	const FVector Up = GetActorUpVector();
	const FVector Right = GetActorRightVector();
	const FVector COM = Body->GetCenterOfMass();

	// Fixed-wing thrust or helicopter collective lift (dead-stick when the tanks are dry).
	const float EffectiveThrottle = FuelPercent > 0.0f ? Throttle : 0.0f;
	if (bHelicopterMode)
	{
		Body->AddForceAtLocation(Up * (EffectiveThrottle * MassKg * 980.0f), COM);
	}
	else
	{
		Body->AddForceAtLocation(Forward * (EffectiveThrottle * MaxThrustN * 100.0f), COM);
	}

	// Ground proximity (drives ground effect + wheel brakes).
	float GroundDistance = 1.0e7f;
	{
		FHitResult GroundHit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(PlaneGround), false, this);
		if (PilotCharacter)
		{
			Params.AddIgnoredActor(PilotCharacter);
		}
		if (GetWorld()->LineTraceSingleByChannel(GroundHit, COM, COM - FVector(0, 0, 1600.0f), ECC_Visibility, Params))
		{
			GroundDistance = GroundHit.Distance;
		}
	}
	const bool bOnGround = GroundDistance < 190.0f;
	if (bWheelBrake && bOnGround && SpeedMps > 0.4f)
	{
		FVector HorizontalVel(Vel.X, Vel.Y, 0.0f);
		if (!HorizontalVel.IsNearlyZero())
		{
			const float BrakeN = MassKg * 5.5f;
			Body->AddForceAtLocation(-HorizontalVel.GetSafeNormal() * BrakeN * 100.0f, COM);
		}
	}

	if (SpeedMps > 2.0f && !bHelicopterMode)
	{
		// Angle of attack from local velocity: nose above the flight path = positive AoA.
		const FVector LocalVel = GetActorTransform().InverseTransformVectorNoScale(Vel);
		const float AoA = FMath::Atan2(-LocalVel.Z, FMath::Max(1.0f, LocalVel.X)); // radians

		float CL = 0.28f + AoA * 4.2f;
		if (bFlapsDown)
		{
			CL += 0.5f;
		}
		// Stall: lift collapses past ~16 degrees AoA.
		const float AoADeg = FMath::Abs(FMath::RadiansToDegrees(AoA));
		bStalled = AoADeg > 16.0f && SpeedMps > 8.0f;
		if (AoADeg > 16.0f)
		{
			CL *= FMath::Clamp(1.0f - (AoADeg - 16.0f) / 22.0f, 0.25f, 1.0f);
		}
		if (bSpoilers)
		{
			CL *= 0.80f; // spoilers dump lift on approach/landing rollout
		}
		CL = FMath::Clamp(CL, -1.2f, 1.9f);

		// Ground effect: extra lift cushion below roughly one wingspan.
		const float GroundEffect = GroundDistance < 1300.0f
			? 1.0f + 0.28f * (1.0f - GroundDistance / 1300.0f) : 1.0f;

		const float DynamicPressure = 0.5f * 1.225f * SpeedMps * SpeedMps;
		const float LiftN = DynamicPressure * WingAreaM2 * CL * GroundEffect;
		Body->AddForceAtLocation(Up * (LiftN * 100.0f), COM);

		float CD = BaseDragCoeff + (bFlapsDown ? 0.05f : 0.0f) + (bGearDown ? 0.035f : 0.0f)
			+ (bSpoilers ? 0.09f : 0.0f) + CL * CL * 0.045f / GroundEffect;
		const float DragN = DynamicPressure * WingAreaM2 * CD;
		Body->AddForceAtLocation(-Vel.GetSafeNormal() * (DragN * 100.0f), COM);
	}

	if (bOccupied)
	{
		// Control authority scales with airspeed (no airflow = no control, like the real thing).
		const float Authority = FMath::Clamp(SpeedMps / 45.0f, 0.12f, 1.3f);
		Body->AddTorqueInRadians(Right * (PitchInput * PitchTorque * Authority), NAME_None, true);
		Body->AddTorqueInRadians(Forward * (-RollInput * RollTorque * Authority), NAME_None, true);
		Body->AddTorqueInRadians(Up * (YawInput * YawTorque * Authority), NAME_None, true);

		// Coordinated-turn helper: banking gently yaws the nose around, arcade-friendly.
		const float Bank = FVector::DotProduct(Right, FVector::UpVector);
		Body->AddTorqueInRadians(FVector::UpVector * (Bank * 0.55f * Authority), NAME_None, true);
	}
}

float AGCPlanePawn::GetSpeedKmh() const
{
	return Body->GetPhysicsLinearVelocity().Size() * 0.036f;
}

bool AGCPlanePawn::BeginEnter(AGCCharacter* Character)
{
	if (!Character || bOccupied)
	{
		return false;
	}
	if (FVector::Distance(GetActorLocation(), Character->GetActorLocation()) > 800.0f)
	{
		return false;
	}
	PilotCharacter = Character;
	bOccupied = true;
	AController* CharController = Character->GetController();
	Character->BeginVehicleTransition();
	Character->SetActorLocation(PilotSeat->GetComponentLocation());
	Character->CompleteEnterVehicle(this, PilotSeat);
	if (CharController)
	{
		CharController->Possess(this);
	}
	return true;
}

void AGCPlanePawn::BeginExit()
{
	if (!bOccupied || !PilotCharacter)
	{
		return;
	}
	// Exiting mid-flight is allowed: that is what the parachute is for.
	AController* PlaneController = GetController();
	PilotCharacter->BeginVehicleTransition();
	PilotCharacter->CompleteExitVehicle(ExitPoint->GetComponentLocation());
	if (PlaneController)
	{
		PlaneController->Possess(PilotCharacter);
	}
	PilotCharacter = nullptr;
	bOccupied = false;
	Throttle = FMath::Min(Throttle, 0.15f);
}

void AGCPlanePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("PlaneThrottleDelta"), this, &AGCPlanePawn::Input_ThrottleDelta);
	PlayerInputComponent->BindAxis(TEXT("PlanePitch"), this, &AGCPlanePawn::Input_Pitch);
	PlayerInputComponent->BindAxis(TEXT("PlaneRoll"), this, &AGCPlanePawn::Input_Roll);
	PlayerInputComponent->BindAxis(TEXT("PlaneYaw"), this, &AGCPlanePawn::Input_Yaw);
	PlayerInputComponent->BindAction(TEXT("Flaps"), IE_Pressed, this, &AGCPlanePawn::Input_ToggleFlaps);
	PlayerInputComponent->BindAction(TEXT("LandingGear"), IE_Pressed, this, &AGCPlanePawn::Input_ToggleGear);
	PlayerInputComponent->BindAction(TEXT("Handbrake"), IE_Pressed, this, &AGCPlanePawn::Input_WheelBrakePressed);
	PlayerInputComponent->BindAction(TEXT("Handbrake"), IE_Released, this, &AGCPlanePawn::Input_WheelBrakeReleased);
	PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, this, &AGCPlanePawn::Input_ToggleSpoilers);
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &AGCPlanePawn::Input_Refuel);
	// Exit on Interact (E), NOT EnterVehicle (F) - F is flaps in a plane.
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AGCPlanePawn::Input_Exit);
}

void AGCPlanePawn::Input_ThrottleDelta(float Value) { ThrottleDeltaInput = Value; }
void AGCPlanePawn::Input_Pitch(float Value) { PitchInput = FMath::Clamp(Value, -1.0f, 1.0f); }
void AGCPlanePawn::Input_Roll(float Value) { RollInput = FMath::Clamp(Value, -1.0f, 1.0f); }
void AGCPlanePawn::Input_Yaw(float Value) { YawInput = FMath::Clamp(Value, -1.0f, 1.0f); }
void AGCPlanePawn::Input_ToggleFlaps() { bFlapsDown = !bFlapsDown; }
void AGCPlanePawn::Input_ToggleGear() { bGearDown = !bGearDown; }
void AGCPlanePawn::Input_ToggleSpoilers() { bSpoilers = !bSpoilers; }
void AGCPlanePawn::Input_WheelBrakePressed() { bWheelBrake = true; }
void AGCPlanePawn::Input_WheelBrakeReleased() { bWheelBrake = false; }
void AGCPlanePawn::Input_Refuel()
{
	if (GetSpeedKmh() < 2.0f)
	{
		FuelPercent = 100.0f;
	}
}
void AGCPlanePawn::Input_Exit() { BeginExit(); }
