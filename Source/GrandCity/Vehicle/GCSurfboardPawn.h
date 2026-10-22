#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GCSurfboardPawn.generated.h"

class USceneComponent;
class UBoxComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class AGCCharacter;
class AGCOceanManager;

UENUM(BlueprintType)
enum class EGCSurfMode : uint8
{
	Surfboard,
	Paddleboard,
	WhitewaterRaft
};

/**
 * Surfboard / paddleboard / raft sharing the main ocean wave field.
 * Surf: wave-slope acceleration + balance/wipeouts. Paddle: stable, paddle-powered.
 * Raft: current + wave push; use foam factor zones for whitewater feel.
 * Kinematic water riding (not rigid-body) keeps it stable and fun.
 */
UCLASS(Blueprintable)
class GRANDCITY_API AGCSurfboardPawn : public APawn
{
	GENERATED_BODY()

public:
	AGCSurfboardPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Surf")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Surf")
	UBoxComponent* InteractionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Surf")
	UStaticMeshComponent* BoardMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Surf")
	USceneComponent* StandingPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Surf|Camera")
	USpringArmComponent* ChaseArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Surf|Camera")
	UCameraComponent* ChaseCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surf")
	EGCSurfMode Mode = EGCSurfMode::Surfboard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surf")
	float WaterBaseLevelZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surf")
	float PaddleAccel = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surf")
	float MaxSpeed = 1900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surf")
	float WaveSlopeAccel = 900.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Surf|State")
	bool bOccupied = false;

	UPROPERTY(BlueprintReadOnly, Category = "Surf|State")
	float Balance = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Surf|State")
	float SpeedKmh = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Surf|State")
	float FoamFactor = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Surf")
	bool BeginRide(AGCCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Surf")
	void EndRide(bool bWipeout = false);

	UFUNCTION(BlueprintCallable, Category = "Surf")
	FString GetModeText() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AGCOceanManager> OceanManager = nullptr;

	UPROPERTY()
	AGCCharacter* Rider = nullptr;

	FVector PlanarVelocity = FVector::ZeroVector;
	float HeadingYaw = 0.0f;
	float InputThrottle = 0.0f;
	float InputSteer = 0.0f;
	float BalanceTimeOverLimit = 0.0f;

	void UpdateRide(float DeltaSeconds);
	bool IsBeached(const FVector& Location, float SurfaceZ) const;

	void Input_Throttle(float Value);
	void Input_Steer(float Value);
	void Input_Exit();
	void Input_CameraTurn(float Value);
	void Input_CameraLookUp(float Value);
};
