#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GCVehicleAIController.generated.h"

class AGCVehiclePawn;

UENUM(BlueprintType)
enum class EGCVehicleAIMode : uint8
{
	Traffic,
	PolicePursuit,
	Parked,
	BusRoute
};

/** Drives an AGCVehiclePawn through its SetAIInputs API: traffic cruising or police pursuit. */
UCLASS()
class GRANDCITY_API AGCVehicleAIController : public AAIController
{
	GENERATED_BODY()

public:
	AGCVehicleAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	EGCVehicleAIMode Mode = EGCVehicleAIMode::Traffic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float TrafficCruiseKmh = 42.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float WaypointAcceptance = 900.0f;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetMode(EGCVehicleAIMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "AI|Bus")
	void SetBusRoute(const TArray<FVector>& RoutePoints, float StopDwellSeconds = 5.0f);

private:
	UPROPERTY()
	AGCVehiclePawn* Vehicle = nullptr;

	FVector CurrentTarget = FVector::ZeroVector;
	bool bHasTarget = false;
	float StuckTimer = 0.0f;
	float ReverseUntilTime = 0.0f;
	float TargetTimer = 0.0f;
	TArray<FVector> BusRoutePoints;
	int32 BusRouteIndex = 0;
	float BusStopDwellSeconds = 5.0f;
	float BusDwellRemaining = 0.0f;

	void PickTrafficTarget();
	void DriveToward(const FVector& TargetLocation, float DesiredKmh, float DeltaSeconds);
};
