#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Core/GCTypes.h"
#include "GCVehiclePawn.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UProceduralMeshComponent;
class UGCDeformationComponent;
class UGCSkidmarkComponent;
class UGCHealthComponent;
class USpringArmComponent;
class UCameraComponent;
class USceneComponent;
class UAudioComponent;
class USpotLightComponent;
class UPointLightComponent;
class UCameraShakeBase;
class UNiagaraComponent;
class UNiagaraSystem;
class AGCCharacter;

USTRUCT(BlueprintType)
struct FGCWheelSetup
{
	GENERATED_BODY()

	/** Anchor position in vehicle local space (X forward, Y right, Z up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LocalOffset = FVector(140.0f, 80.0f, 30.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Radius = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Width = 26.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSteering = false;
};

USTRUCT(BlueprintType)
struct FGCVehicleTireCondition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Tyres", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Integrity = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Tyres")
    bool bPunctured = false;

    UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Tyres", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GripScale = 1.0f;
};

UENUM(BlueprintType)
enum class EGCVehicleState : uint8
{
	Empty,
	Entering,
	Occupied,
	Exiting
};

/**
 * Raycast vehicle pawn (the core-sandbox centrepiece).
 *
 * Physics model, all in C++:
 *  - Per-wheel raycast suspension (spring + damper), anti-roll bars, downforce, aero drag.
 *  - Tyres: lateral force from slip velocity with saturation (tanh curve), longitudinal force
 *    clipped by a friction circle, rolling resistance, brake & handbrake (handbrake slashes
 *    rear lateral grip = drifting), wheelspin when demand exceeds available grip.
 *  - Powertrain: engine torque curve shape, automatic gearbox, FWD/RWD/AWD, RPM from drive wheels.
 *  - Speed-sensitive steering with Ackermann Ackermann for the steered axle.
 *  - Crash response: impact energy dents the body through UGCDeformationComponent, applies
 *    vehicle damage, camera shake and FX/audio hooks.
 *  - Animated driver door + character seat transition for enter/exit.
 *  - Any imported car static mesh (e.g. a free Sketchfab model) can be dropped onto BodyMesh;
 *    just adjust wheel offsets/box extent in a Blueprint child. No skeletal rig required.
 */
UCLASS(Blueprintable)
class GRANDCITY_API AGCVehiclePawn : public APawn
{
	GENERATED_BODY()

public:
	AGCVehiclePawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ---------------------------------------------------------------- components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	UBoxComponent* CollisionBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	UStaticMeshComponent* BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	UProceduralMeshComponent* DeformedBodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	UGCDeformationComponent* Deformation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	UGCSkidmarkComponent* Skidmarks;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	UGCHealthComponent* Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Camera")
	USpringArmComponent* ChaseArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Camera")
	UCameraComponent* ChaseCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Camera")
	UCameraComponent* CockpitCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	USceneComponent* DriverSeat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	USceneComponent* ExitPoint;

	/** Driver door hinge. Attach your door mesh in a Blueprint child (or leave empty). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	USceneComponent* DriverDoorPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	TArray<UStaticMeshComponent*> WheelMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Audio")
	UAudioComponent* EngineAudio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Audio")
	UAudioComponent* ScreechAudio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Audio")
	UAudioComponent* SirenAudio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Lights")
	USpotLightComponent* HeadlightL;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Lights")
	USpotLightComponent* HeadlightR;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Lights")
	UPointLightComponent* SirenLightRed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Lights")
	UPointLightComponent* SirenLightBlue;

	// ---------------------------------------------------------------- tuning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Wheels")
	TArray<FGCWheelSetup> Wheels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Powertrain")
	EGCVehicleDriveType DriveType = EGCVehicleDriveType::RWD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Powertrain")
	float MassKg = 1450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Powertrain")
	float EngineMaxTorqueNm = 430.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Powertrain")
	float MaxRPM = 6500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Powertrain")
	float IdleRPM = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Powertrain")
	TArray<float> GearRatios = { 3.4f, 2.3f, 1.7f, 1.3f, 1.05f, 0.85f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Powertrain")
	float FinalDriveRatio = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Powertrain")
	float ReverseGearRatio = 3.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
	float SuspensionRestLength = 42.0f;

	/** N/m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
	float SpringRateNPerM = 38000.0f;

	/** N/(m/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
	float DampingNPerMS = 4600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
	float AntiRollStiffness = 5200.0f;

	/** Peak tyre friction coefficient (asphalt ~1.0-1.3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tyres")
	float TireFrictionMu = 1.15f;

	/** How aggressively lateral force builds with slip velocity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tyres")
	float LateralGripStiffness = 0.42f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tyres")
	float RollingResistanceCoeff = 90.0f;

	/** Total brake force (N) across all wheels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tyres")
	float MaxBrakeForceN = 26000.0f;

	/** Rear lateral-grip multiplier while the handbrake is held (drift!). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tyres")
	float HandbrakeRearGripMultiplier = 0.22f;

    /** Per-wheel condition state. Index order matches the Wheels setup array. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Tyres")
    TArray<FGCVehicleTireCondition> TireConditions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tyres", meta = (ClampMin = "0.0"))
    float TireImpactDamageScale = 0.00008f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tyres", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PuncturedTireMinimumGrip = 0.22f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Powertrain", meta = (ClampMin = "40.0"))
    float InitialEngineTemperatureC = 88.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Powertrain", meta = (ClampMin = "50.0"))
    float EngineOverheatTemperatureC = 112.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Powertrain", meta = (ClampMin = "60.0"))
    float EngineCriticalTemperatureC = 135.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Powertrain", meta = (ClampMin = "0.0"))
    float RadiatorCoolingRate = 0.012f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Powertrain", meta = (ClampMin = "0.0"))
    float EngineHeatPerThrottle = 3.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Powertrain")
    float EngineTemperatureC = 88.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Powertrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RadiatorIntegrity = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Powertrain")
    bool bEngineStalled = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Powertrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EngineMisfireSeverity = 0.0f;

    /** Optional Niagara smoke system; assign an engine-bay plume in a vehicle Blueprint. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Damage|VFX")
    TObjectPtr<UNiagaraSystem> EngineSmokeSystem = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Damage")
    void DamageTire(int32 WheelIndex, float DamageAmount);

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Damage")
    void RepairTires();

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Damage")
    void DamageRadiator(float DamageAmount);

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Damage")
    void RepairMechanicalSystems();

    UFUNCTION(BlueprintCallable, Category = "Police|Traffic")
    bool BeginTrafficStop(AGCCharacter* Officer);

    UFUNCTION(BlueprintCallable, Category = "Police|Traffic")
    bool RunBreathalyzer(AGCCharacter* Officer);

    UFUNCTION(BlueprintCallable, Category = "Police|Traffic")
    bool RunDrugImpairmentTest(AGCCharacter* Officer);

    UFUNCTION(BlueprintCallable, Category = "Police|Traffic")
    bool IssueTrafficCitation(AGCCharacter* Officer, EGCPoliceViolation Violation);

    UFUNCTION(BlueprintCallable, Category = "Police|Traffic")
    void ReleaseTrafficStop(AGCCharacter* Officer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Aero")
	float DragCoeff = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Aero")
	float DownforceCoeff = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Steering")
	float MaxSteerAngleDegrees = 34.0f;

	/** Steer limit at 180 km/h (degrees). Interpolated by speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Steering")
	float HighSpeedSteerAngleDegrees = 9.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Steering")
	float SteerSpeed = 5.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Crash")
	float MinCrashSpeedForDent = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Crash")
	TSubclassOf<UCameraShakeBase> CrashShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Police")
	bool bIsPoliceVehicle = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Ownership")
    bool bIsPlayerOwned = false;

    UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Ownership")
    bool bWasStolen = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Ownership")
    FString VehicleID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Ownership")
    FString DisplayName = TEXT("Personal vehicle");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Ownership")
    FString LicensePlate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Traffic")
    bool bExpiredMeter = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Traffic")
    bool bParkedOnSidewalk = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Traffic")
    bool bBlockingFireHydrant = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Traffic")
    bool bAgainstTraffic = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Traffic")
    bool bCommittedIllegalTurn = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Traffic")
    bool bTailLightsFunctional = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Traffic")
    bool bSmokingExhaust = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Traffic")
    bool bRecklessDriving = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Traffic")
    bool bDriverImpaired = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Traffic")
    bool bDriverDrugImpaired = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Traffic")
    bool bDriverSlurredSpeech = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Traffic")
    bool bDriverUnsteadyGait = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Traffic")
    bool bDriverHasValidLicense = true;

    UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Traffic")
    bool bTrafficStopped = false;

    UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Traffic")
    float TrafficStopSpeedKmh = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Debug")
	bool bDebugDrawWheels = false;

	/** Visual roll offset (deg) for wheel meshes whose axle is not modelled on Y.
	 *  90 suits the fallback engine cylinder; set 0 for typical imported wheel meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Wheels")
	float WheelVisualRollOffsetDeg = 90.0f;

	// ---------------------------------------------------------------- runtime state (HUD/AI read these)
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|State")
	EGCVehicleState VehicleState = EGCVehicleState::Empty;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|State")
	float CurrentRPM = 900.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|State")
	int32 CurrentGear = 1; // 0 = neutral, -1 = reverse, 1..N forward

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|State")
	bool bDrifting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|State")
	bool bWrecked = false;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|State")
	EGCCameraMode CameraMode = EGCCameraMode::Chase;

	UFUNCTION(BlueprintCallable, Category = "Vehicle|State")
	float GetSpeedKmh() const;

	UFUNCTION(BlueprintCallable, Category = "Vehicle|State")
	float GetForwardSpeedCms() const;

	/** -1..1 steering deflection for seated-character hand/wheel animation. */
	UFUNCTION(BlueprintCallable, Category = "Vehicle|State")
	float GetSteeringNormalized() const;

	/** AI / external driver input. Values are held until changed. */
	UFUNCTION(BlueprintCallable, Category = "Vehicle|Input")
	void SetAIInputs(float Throttle, float Steering, bool bInHandbrake);

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Occupancy")
	bool BeginEnter(AGCCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Occupancy")
	void BeginExit();

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Occupancy")
	AGCCharacter* GetDriver() const { return DriverCharacter; }

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Lights")
	void SetSiren(bool bOn);

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Lights")
	void SetHeadlights(bool bOn);

protected:
	UFUNCTION()
	void OnBodyHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnVehicleDeath(AActor* Killer);

private:
	struct FWheelRuntime
	{
		bool bGrounded = false;
		FVector ContactPoint = FVector::ZeroVector;
		FVector ContactNormal = FVector::UpVector;
		float Compression = 0.0f;      // m
		float SuspensionForceN = 0.0f; // N
		float SteerAngleRad = 0.0f;
		float SpinAngleDeg = 0.0f;
		float LateralSlipMps = 0.0f;
		float LongitudinalSpeedMps = 0.0f;
		bool bDrive = false;
		bool bSlipping = false;
	};

	TArray<FWheelRuntime> WheelState;

	float InputThrottle = 0.0f;
	float InputSteering = 0.0f;
	bool bInputHandbrake = false;
	float CurrentSteerAngleRad = 0.0f;
	float ShiftTimer = 0.0f;
    float EngineMisfireTimer = 0.0f;
    float EngineSmokeUpdateTimer = 0.0f;
    UPROPERTY(Transient)
    TObjectPtr<UNiagaraComponent> EngineSmokeComponent = nullptr;
	bool bSirenOn = false;
	float SirenFlashTimer = 0.0f;
	bool bSirenPhase = false;
	bool bHeadlightsOn = false;

	UPROPERTY()
	AGCCharacter* DriverCharacter = nullptr;

	float OccupancyTimer = 0.0f;
	FVector EnterStartLocation = FVector::ZeroVector;
	FRotator EnterStartRotation = FRotator::ZeroVector;
	float DoorOpenAlpha = 0.0f; // 0 closed, 1 open

	void UpdateVehiclePhysics(float DeltaSeconds);
    void UpdateMechanicalSystems(float DeltaSeconds);
    void UpdateEngineSmoke();
	void ApplyWheelForces(float DeltaSeconds);
	void UpdatePowertrain(float DeltaSeconds, float& OutTotalDriveForceN);
	void UpdateWheelVisuals(float DeltaSeconds);
	void UpdateAudioAndLights(float DeltaSeconds);
	void UpdateOccupancy(float DeltaSeconds);
	void SetDoorAngle(float Alpha);

	FVector GetPointVelocity(const FVector& WorldPoint) const;
	bool IsDriveWheel(int32 Index) const;
	float GetMaxSteerRad() const;

	// Player input handlers
	void Input_Throttle(float Value);
	void Input_Steer(float Value);
	void Input_HandbrakePressed();
	void Input_HandbrakeReleased();
	void Input_ExitPressed();
	void Input_ToggleCamera();
	void Input_ToggleLights();
	void Input_ToggleSiren();
	void Input_CameraTurn(float Value);
	void Input_CameraLookUp(float Value);
};
