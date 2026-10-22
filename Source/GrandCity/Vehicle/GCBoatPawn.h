#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GCBoatPawn.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UAudioComponent;
class UNiagaraSystem;
class USceneComponent;
class USpringArmComponent;
class UCameraComponent;
class AGCCharacter;
class AGCOceanManager;

/** Boat with multi-point buoyancy sampled from the shared ocean wave field. */
UCLASS(Blueprintable)
class GRANDCITY_API AGCBoatPawn : public APawn
{
	GENERATED_BODY()

public:
	AGCBoatPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat")
	UBoxComponent* HullBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat")
	UStaticMeshComponent* HullMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat")
	USceneComponent* DriverSeat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat")
	USceneComponent* ExitPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Camera")
	USpringArmComponent* ChaseArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Camera")
	UCameraComponent* ChaseCamera;

	/** Buoyancy sample points, local space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Buoyancy")
	TArray<FVector> BuoyancyPoints;

	/** Newtons of lift per cm of submersion, per point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Buoyancy")
	float BuoyancyPerCm = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Buoyancy")
	float WaterDrag = 0.35f;

	/** Assumed ocean base level Z (the ocean actor's Z). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Buoyancy")
	float WaterBaseLevelZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Propulsion")
	float EngineForceN = 16000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Propulsion")
	float RudderTorque = 2.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat")
	float MassKg = 900.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Audio")
	UAudioComponent* EngineAudio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|FX")
	UNiagaraSystem* WakeFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|FX")
	UNiagaraSystem* SprayFX = nullptr;

	/** Speed (m/s) where the hull starts to plane and lift out of the water. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Propulsion")
	float PlaningStartSpeedMps = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Propulsion")
	float PlaningLiftCoeff = 55.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Boat|State")
	bool bAnchored = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boat|State")
	bool bPlaning = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boat|State")
	float LastFoamFactor = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Boat")
	float GetSpeedKmh() const;

	UFUNCTION(BlueprintCallable, Category = "Boat")
	bool BeginEnter(AGCCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Boat")
	void BeginExit();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<AGCOceanManager> OceanManager = nullptr;

	UPROPERTY()
	AGCCharacter* DriverCharacter = nullptr;

	bool bOccupied = false;
	float InputThrottle = 0.0f;
	float InputSteer = 0.0f;
	float SmoothedThrottle = 0.0f;
	float WakeTimer = 0.0f;
	int32 SubmergedPoints = 0;

	void ApplyBuoyancyAndPropulsion(float DeltaSeconds);
	FVector GetPointVelocity(const FVector& WorldPoint) const;

	void Input_Throttle(float Value);
	void Input_Steer(float Value);
	void Input_Anchor();
	void Input_Exit();
	void Input_CameraTurn(float Value);
	void Input_CameraLookUp(float Value);
};
