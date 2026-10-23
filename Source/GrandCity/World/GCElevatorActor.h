#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCElevatorActor.generated.h"

class AGCCharacter;
class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UGCHotelElevatorScreenWidget;

UENUM(BlueprintType)
enum class EGCElevatorState : uint8
{
	Idle,
	OpeningDoors,
	Boarding,
	Dwelling,
	ClosingDoors,
	Moving
};

/** Single-car, single-shaft elevator with moving-base collision and animated split doors. */
UCLASS(Blueprintable)
class GRANDCITY_API AGCElevatorActor : public AActor
{
	GENERATED_BODY()

public:
	AGCElevatorActor();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Elevator")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Elevator")
	UStaticMeshComponent* CarFloor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Elevator")
	UStaticMeshComponent* LeftDoor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Elevator")
	UStaticMeshComponent* RightDoor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Elevator")
	UStaticMeshComponent* LeftCabinWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Elevator")
	UStaticMeshComponent* RightCabinWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Elevator")
	UStaticMeshComponent* BackCabinWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Elevator|Safety")
	UBoxComponent* DoorSafetySensor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Elevator|Display")
	UWidgetComponent* CabinDisplay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator|Display", meta = (ClampMin = "100.0", ClampMax = "1000.0"))
	float FloorHeightCm = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator|Motion", meta = (ClampMin = "100.0", ClampMax = "600.0"))
	float TravelSpeedCmPerSecond = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator|Motion", meta = (ClampMin = "40.0", ClampMax = "180.0"))
	float DoorSpeedCmPerSecond = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator|Motion", meta = (ClampMin = "1.0", ClampMax = "8.0"))
	float DoorDwellSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator|Motion", meta = (ClampMin = "5.0", ClampMax = "60.0"))
	float BoardingTimeoutSeconds = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator|Geometry", meta = (ClampMin = "30.0", ClampMax = "120.0"))
	float DoorOpenDistanceCm = 85.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Elevator|State")
	EGCElevatorState State = EGCElevatorState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Elevator|State")
	float DoorAlpha = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Elevator")
	bool CallFromLanding(AActor* CallerLanding, AActor* DestinationLanding, AGCCharacter* Passenger);

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<AActor> RequestedFloor;
	TWeakObjectPtr<AGCCharacter> RequestedPassenger;
	UPROPERTY(Transient)
	TObjectPtr<UGCHotelElevatorScreenWidget> ScreenWidget;
	float BaseFloorZ = 0.0f;
	float TargetZ = 0.0f;
	float BoardingFloorZ = 0.0f;
	float DestinationFloorZ = 0.0f;
	float DwellRemaining = 0.0f;
	float BoardingRemaining = 0.0f;
	bool bGoingToPickupFloor = false;
	bool bAtPickupFloor = false;
	bool bFloorSelectionDisplayed = false;
	bool IsPassengerInsideCabin(const AGCCharacter* Passenger) const;
	void SetDoorAlpha(float NewAlpha);
};
