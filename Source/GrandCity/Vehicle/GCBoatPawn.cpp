#include "Vehicle/GCBoatPawn.h"
#include "Camera/CameraComponent.h"
#include "Character/GCCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "World/GCOceanManager.h"

AGCBoatPawn::AGCBoatPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	HullBody = CreateDefaultSubobject<UBoxComponent>(TEXT("HullBody"));
	RootComponent = HullBody;
	HullBody->SetBoxExtent(FVector(320.0f, 120.0f, 55.0f));
	HullBody->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	HullBody->SetSimulatePhysics(true);
	HullBody->SetLinearDamping(0.25f);
	HullBody->SetAngularDamping(0.9f);

	HullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullMesh"));
	HullMesh->SetupAttachment(HullBody);
	HullMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HullMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -55.0f));

	DriverSeat = CreateDefaultSubobject<USceneComponent>(TEXT("DriverSeat"));
	DriverSeat->SetupAttachment(HullBody);
	DriverSeat->SetRelativeLocation(FVector(-60.0f, 0.0f, 45.0f));

	ExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ExitPoint"));
	ExitPoint->SetupAttachment(HullBody);
	ExitPoint->SetRelativeLocation(FVector(-60.0f, -230.0f, 60.0f));

	ChaseArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ChaseArm"));
	ChaseArm->SetupAttachment(HullBody);
	ChaseArm->SetRelativeLocation(FVector(0.0f, 0.0f, 140.0f));
	ChaseArm->TargetArmLength = 850.0f;
	ChaseArm->bUsePawnControlRotation = true;
	ChaseArm->bEnableCameraLag = true;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(ChaseArm);

	EngineAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));
	EngineAudio->SetupAttachment(HullBody);
	EngineAudio->bAutoActivate = false;

	BuoyancyPoints = {
		FVector(260.0f, -80.0f, -30.0f), FVector(260.0f, 80.0f, -30.0f),
		FVector(0.0f, -95.0f, -30.0f),   FVector(0.0f, 95.0f, -30.0f),
		FVector(-260.0f, -80.0f, -30.0f), FVector(-260.0f, 80.0f, -30.0f)
	};
}

FVector AGCBoatPawn::GetPointVelocity(const FVector& WorldPoint) const
{
	return HullBody->GetPhysicsLinearVelocity()
		+ FVector::CrossProduct(HullBody->GetPhysicsAngularVelocityInRadians(),
			WorldPoint - HullBody->GetCenterOfMass());
}

void AGCBoatPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ApplyBuoyancyAndPropulsion(DeltaSeconds);
}

void AGCBoatPawn::ApplyBuoyancyAndPropulsion(float DeltaSeconds)
{
	if (!HullBody->IsSimulatingPhysics() || !GetWorld())
	{
		return;
	}
	HullBody->SetMassOverrideInKg(NAME_None, MassKg, false);

	const float Time = GetWorld()->GetTimeSeconds();
	const FTransform TM = HullBody->GetComponentTransform();
	SubmergedPoints = 0;

	float FoamAccum = 0.0f;
	for (const FVector& LocalPoint : BuoyancyPoints)
	{
		const FVector WorldPoint = TM.TransformPosition(LocalPoint);
		const FGCOceanSample Sample = AGCOceanManager::SampleOceanStatic(
			FVector2D(WorldPoint.X, WorldPoint.Y), Time, 1.0f);
		const float SurfaceZ = WaterBaseLevelZ + Sample.Height;
		FoamAccum += Sample.FoamFactor;
		const float DepthCm = SurfaceZ - WorldPoint.Z;
		if (DepthCm <= 0.0f)
		{
			continue;
		}
		++SubmergedPoints;
		const float ClampedDepth = FMath::Min(DepthCm, 130.0f);
		float ForceN = ClampedDepth * BuoyancyPerCm;
		// Damp relative to the moving surface, so the hull rides swells rather than bouncing off them.
		const float RelativeVerticalVel = GetPointVelocity(WorldPoint).Z - Sample.VerticalVelocity;
		ForceN -= RelativeVerticalVel * 6.0f;
		ForceN = FMath::Max(0.0f, ForceN);
		HullBody->AddForceAtLocation(FVector(0.0f, 0.0f, ForceN * 100.0f), WorldPoint);
	}
	LastFoamFactor = BuoyancyPoints.Num() > 0 ? FoamAccum / BuoyancyPoints.Num() : 0.0f;
	SmoothedThrottle = FMath::FInterpTo(SmoothedThrottle, bAnchored ? 0.0f : InputThrottle, DeltaSeconds, 2.2f);

	if (SubmergedPoints > 0)
	{
		// Water drag opposing motion (stronger laterally, so boats do not slide like cars on ice).
		const FVector Vel = HullBody->GetPhysicsLinearVelocity();
		const FVector Forward = GetActorForwardVector();
		const FVector Right = GetActorRightVector();
		const float FwdVel = FVector::DotProduct(Vel, Forward);
		const float LatVel = FVector::DotProduct(Vel, Right);
		const FVector DragForce = Forward * (-FwdVel * WaterDrag * MassKg * 0.55f)
			+ Right * (-LatVel * WaterDrag * MassKg * 2.4f);
		HullBody->AddForceAtLocation(DragForce * 100.0f / 100.0f, HullBody->GetCenterOfMass());

		// Ocean current pushes the hull toward the surface flow.
		const FVector Current = AGCOceanManager::GetCurrentAt(
			FVector2D(GetActorLocation().X, GetActorLocation().Y), Time);
		const FVector HorizontalVel(Vel.X, Vel.Y, 0.0f);
		HullBody->AddForceAtLocation((Current - HorizontalVel) * MassKg * 0.045f, HullBody->GetCenterOfMass());

		// Planing: above hull speed the stern generates dynamic lift and the bow rises.
		const float FwdMps = FwdVel / 100.0f;
		bPlaning = FwdMps > PlaningStartSpeedMps;
		if (bPlaning)
		{
			const float Over = FwdMps - PlaningStartSpeedMps;
			const float LiftN = FMath::Min(Over * Over * PlaningLiftCoeff, MassKg * 4.5f);
			HullBody->AddForceAtLocation(FVector(0.0f, 0.0f, LiftN * 100.0f),
				TM.TransformPosition(FVector(-230.0f, 0.0f, -20.0f)));
		}

		// Wake + bow spray FX (Niagara systems assigned in a Blueprint child).
		WakeTimer -= DeltaSeconds;
		if (WakeTimer <= 0.0f && FMath::Abs(FwdMps) > 2.5f)
		{
			WakeTimer = 0.12f;
			if (WakeFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), WakeFX,
					TM.TransformPosition(FVector(-300.0f, 0.0f, 0.0f)), GetActorRotation());
			}
			if (SprayFX && (LastFoamFactor > 0.30f || bPlaning))
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SprayFX,
					TM.TransformPosition(FVector(240.0f, 0.0f, 0.0f)), GetActorRotation());
			}
		}

		// Propulsion + rudder.
		if (bOccupied && !bAnchored)
		{
			HullBody->AddForceAtLocation(Forward * (SmoothedThrottle * EngineForceN * 100.0f),
				TM.TransformPosition(FVector(-280.0f, 0.0f, -10.0f)));
			const float SpeedFactor = FMath::Clamp(FMath::Abs(FwdVel) / 600.0f, 0.15f, 1.0f);
			const float YawTorque = -InputSteer * RudderTorque * SpeedFactor * (FwdVel >= 0.0f ? 1.0f : -1.0f);
			HullBody->AddTorqueInRadians(FVector(0.0f, 0.0f, YawTorque), NAME_None, true);
		}
	}
	else
	{
		bPlaning = false;
	}

	// Engine audio.
	if (EngineAudio && EngineAudio->GetSound())
	{
		if (bOccupied && SubmergedPoints > 0)
		{
			if (!EngineAudio->IsPlaying())
			{
				EngineAudio->Play();
			}
			EngineAudio->SetPitchMultiplier(0.75f + FMath::Abs(SmoothedThrottle) * 0.9f);
			EngineAudio->SetVolumeMultiplier(0.6f + FMath::Abs(SmoothedThrottle) * 0.4f);
		}
		else if (EngineAudio->IsPlaying())
		{
			EngineAudio->Stop();
		}
	}
}

float AGCBoatPawn::GetSpeedKmh() const
{
	return HullBody->GetPhysicsLinearVelocity().Size() * 0.036f;
}

bool AGCBoatPawn::BeginEnter(AGCCharacter* Character)
{
	if (!Character || bOccupied)
	{
		return false;
	}
	if (FVector::Distance(GetActorLocation(), Character->GetActorLocation()) > 650.0f)
	{
		return false;
	}
	DriverCharacter = Character;
	bOccupied = true;
	AController* CharController = Character->GetController();
	Character->BeginVehicleTransition();
	Character->SetActorLocation(DriverSeat->GetComponentLocation());
	Character->CompleteEnterVehicle(this, DriverSeat);
	if (CharController)
	{
		CharController->Possess(this);
	}
	return true;
}

void AGCBoatPawn::BeginExit()
{
	if (!bOccupied || !DriverCharacter)
	{
		return;
	}
	AController* BoatController = GetController();
	DriverCharacter->BeginVehicleTransition();
	DriverCharacter->CompleteExitVehicle(ExitPoint->GetComponentLocation() + FVector(0, 0, 40.0f));
	if (BoatController)
	{
		BoatController->Possess(DriverCharacter);
	}
	DriverCharacter = nullptr;
	bOccupied = false;
	InputThrottle = 0.0f;
	InputSteer = 0.0f;
}

void AGCBoatPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("VehicleThrottle"), this, &AGCBoatPawn::Input_Throttle);
	PlayerInputComponent->BindAxis(TEXT("VehicleSteer"), this, &AGCBoatPawn::Input_Steer);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AGCBoatPawn::Input_CameraTurn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AGCBoatPawn::Input_CameraLookUp);
	PlayerInputComponent->BindAction(TEXT("Handbrake"), IE_Pressed, this, &AGCBoatPawn::Input_Anchor);
	PlayerInputComponent->BindAction(TEXT("EnterVehicle"), IE_Pressed, this, &AGCBoatPawn::Input_Exit);
}

void AGCBoatPawn::Input_Throttle(float Value) { InputThrottle = FMath::Clamp(Value, -1.0f, 1.0f); }
void AGCBoatPawn::Input_Steer(float Value) { InputSteer = FMath::Clamp(Value, -1.0f, 1.0f); }
void AGCBoatPawn::Input_Anchor() { bAnchored = !bAnchored; }
void AGCBoatPawn::Input_Exit() { BeginExit(); }
void AGCBoatPawn::Input_CameraTurn(float Value) { AddControllerYawInput(Value); }
void AGCBoatPawn::Input_CameraLookUp(float Value) { AddControllerPitchInput(Value); }
