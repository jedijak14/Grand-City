#pragma once

#include "CoreMinimal.h"
#include "Vehicle/GCVehiclePawn.h"
#include "GCBusPawn.generated.h"

class AGCBusStopActor;
class AGCCharacter;
class USceneComponent;

/** Autonomous route bus with a separate passenger seat; it remains AI-driven while players ride. */
UCLASS(Blueprintable)
class GRANDCITY_API AGCBusPawn : public AGCVehiclePawn
{
	GENERATED_BODY()

public:
	AGCBusPawn();
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bus")
	USceneComponent* PassengerSeat;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Bus|Route")
	TArray<AGCBusStopActor*> RouteStops;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bus|Route", meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float StopDwellSeconds = 7.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bus|Route", meta = (ClampMin = "5.0", ClampMax = "80.0"))
    float RouteCruiseKmh = 38.0f;

    UFUNCTION(BlueprintCallable, Category = "Bus|Route")
    float GetEstimatedArrivalSeconds(AGCBusStopActor* Stop) const;

	UFUNCTION(BlueprintCallable, Category = "Bus")
	bool BoardPassenger(AGCCharacter* Character, AGCBusStopActor* DestinationStop, int32 Fare);

	UFUNCTION(BlueprintCallable, Category = "Bus")
	void NotifyRouteStopReached(int32 StopIndex);

	UFUNCTION(BlueprintCallable, Category = "Bus")
	void DisembarkPassenger();

private:
	UPROPERTY(Transient)
	TObjectPtr<AGCCharacter> PassengerCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AGCBusStopActor> PassengerDestination = nullptr;
    int32 CurrentRouteTargetIndex = 0;
};
