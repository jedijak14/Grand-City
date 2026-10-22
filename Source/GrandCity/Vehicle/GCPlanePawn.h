#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GCPlanePawn.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USceneComponent;
class USpringArmComponent;
class UCameraComponent;
class UAudioComponent;
class UGCHealthComponent;
class AGCCharacter;
class AGCOceanManager;

/**
 * Flyable fixed-wing plane: thrust, lift from angle of attack + airspeed,
 * flaps (more lift, more drag), retractable gear (drag), pitch/roll/rudder torques.
 * Takeoff is earned: accelerate down the runway until lift beats weight.
 */
UCLASS(Blueprintable)
class GRANDCITY_API AGCPlanePawn : public APawn
{
	GENERATED_BODY()

public:
	AGCPlanePawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plane")
	UBoxComponent* Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plane")
	UStaticMeshComponent* FuselageMesh;

	/** Spins with throttle (assign a propeller mesh in a BP child for prop planes). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plane")
	UStaticMeshComponent* PropellerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plane")
	USceneComponent* PilotSeat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plane")
	USceneComponent* ExitPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plane|Camera")
	USpringArmComponent* ChaseArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plane|Camera")
	UCameraComponent* ChaseCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plane")
	UGCHealthComponent* Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plane|Audio")
	UAudioComponent* EngineAudio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane|Aero")
	float MassKg = 1150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane|Aero")
	float WingAreaM2 = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane|Aero")
	float MaxThrustN = 7200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane|Aero")
	float BaseDragCoeff = 0.045f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane|Controls")
	float PitchTorque = 1.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane|Controls")
	float RollTorque = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane|Controls")
	float YawTorque = 0.9f;

	/** 0..1, adjusted with Shift/Ctrl. */
	UPROPERTY(BlueprintReadOnly, Category = "Plane|State")
	float Throttle = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Plane|State")
	bool bFlapsDown = false;

	UPROPERTY(BlueprintReadOnly, Category = "Plane|State")
	bool bGearDown = true;

	UPROPERTY(BlueprintReadOnly, Category = "Plane|State")
	bool bOccupied = false;

	UPROPERTY(BlueprintReadOnly, Category = "Plane|State")
	bool bSpoilers = false;

	UPROPERTY(BlueprintReadOnly, Category = "Plane|State")
	bool bWheelBrake = false;

	UPROPERTY(BlueprintReadOnly, Category = "Plane|State")
	bool bStalled = false;

	/** 0-100 percent. Burn scales with throttle; refuel on the ground with R. */
	UPROPERTY(BlueprintReadOnly, Category = "Plane|State")
	float FuelPercent = 82.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane|Aero")
	float FuelBurnPercentPerSecond = 0.30f;

	UFUNCTION(BlueprintCallable, Category = "Plane")
	float GetSpeedKmh() const;

	UFUNCTION(BlueprintCallable, Category = "Plane")
	bool BeginEnter(AGCCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Plane")
	void BeginExit();

private:
	UPROPERTY()
	AGCCharacter* PilotCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AGCOceanManager> OceanManager = nullptr;

	float WaterWakeTimer = 0.0f;
	float ThrottleDeltaInput = 0.0f;
	float PitchInput = 0.0f;
	float RollInput = 0.0f;
	float YawInput = 0.0f;
	float PropSpinDeg = 0.0f;

	void ApplyFlightPhysics(float DeltaSeconds);

	void Input_ThrottleDelta(float Value);
	void Input_Pitch(float Value);
	void Input_Roll(float Value);
	void Input_Yaw(float Value);
	void Input_ToggleFlaps();
	void Input_ToggleGear();
	void Input_ToggleSpoilers();
	void Input_WheelBrakePressed();
	void Input_WheelBrakeReleased();
	void Input_Refuel();
	void Input_Exit();
};
