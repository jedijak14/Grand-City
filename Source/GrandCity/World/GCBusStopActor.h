#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCBusStopActor.generated.h"

class AGCCharacter;
class UBoxComponent;
class UStaticMeshComponent;

/** Prototype city-bus stop. A fare-paying route ride transfers the player to a linked stop. */
UCLASS(Blueprintable)
class GRANDCITY_API AGCBusStopActor : public AActor
{
	GENERATED_BODY()

public:
	AGCBusStopActor();
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bus Stop")
	UBoxComponent* Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bus Stop")
	UStaticMeshComponent* StopSign;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bus Stop|Route")
	FString StopName = TEXT("Downtown Central");

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Bus Stop|Route")
	TArray<AGCBusStopActor*> DestinationStops;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bus Stop|Route", meta = (ClampMin = "0"))
	int32 Fare = 25;

	UFUNCTION(BlueprintCallable, Category = "Bus Stop")
	void CycleDestination();

	UFUNCTION(BlueprintCallable, Category = "Bus Stop")
	void Ride(AGCCharacter* Passenger);

protected:
	UFUNCTION()
	void OnTriggerBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);

	UFUNCTION()
	void OnTriggerEnd(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	int32 SelectedDestinationIndex = 0;
	TWeakObjectPtr<AGCCharacter> CurrentPassenger;
};
