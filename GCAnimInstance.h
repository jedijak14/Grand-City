#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GCAnimInstance.generated.h"

/** Drives locomotion blend spaces for the player and pedestrians. */
UCLASS(Blueprintable)
class GRANDCITY_API UGCAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bSeated = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bParachuting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bSwimming = false;

	/** Yaw offset of movement vs facing, for aim/strafe blend spaces. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.0f;

	/** Local velocity (X forward, Y right, cm/s) for 2D blend spaces. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	FVector2D LocalVelocity2D = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float AccelerationSize = 0.0f;

	/** Side-to-side body lean from lateral acceleration (-1..1). */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float BodyLean = 0.0f;

	/** Aim offsets in degrees, already clamped. */
	UPROPERTY(BlueprintReadOnly, Category = "Aiming")
	float AimPitch = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aiming")
	float AimYaw = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bCrouching = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bDead = false;

	/** Steering-wheel deflection for seated driving animation (-1..1). */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle")
	float VehicleSteering = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float TurnYawRate = 0.0f;

private:
	float LastDirection = 0.0f;
};
