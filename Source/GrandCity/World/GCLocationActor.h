#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GCTypes.h"
#include "GCLocationActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AGCCharacter;
class AGCBoatPawn;

/**
 * Interactable world location: hospital (heal), shop (rob it - cash + wanted),
 * police station (surrender/pay fine), airport (booked flight to a linked airport),
 * marina (rent a boat), skyscraper elevator (to the roof - bring a parachute).
 */
UCLASS(Blueprintable)
class GRANDCITY_API AGCLocationActor : public AActor
{
	GENERATED_BODY()

public:
	AGCLocationActor();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Location")
	UBoxComponent* Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Location")
	UStaticMeshComponent* BuildingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Location")
	UStaticMeshComponent* SignMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	EGCLocationType LocationType = EGCLocationType::Shop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	FText InteractionText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location|Shop")
	int32 RobRewardMin = 400;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location|Shop")
	int32 RobRewardMax = 1600;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location|Shop")
	float RobCooldownSeconds = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location|Hospital")
	int32 HealCost = 200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location|Airport")
	int32 FlightCost = 350;

	/** Airport-to-airport link for booked passenger flights. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location|Airport")
	AGCLocationActor* FlightDestination = nullptr;

	/** Elevator destination (roof marker actor). */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location|Elevator")
	AActor* TeleportDestination = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Location|Marina")
	TSubclassOf<AGCBoatPawn> RentalBoatClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location|Marina")
	int32 BoatRentalCost = 150;

	UFUNCTION(BlueprintCallable, Category = "Location")
	void Interact(AGCCharacter* Character);

protected:
	UFUNCTION()
	void OnTriggerBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);

	UFUNCTION()
	void OnTriggerEnd(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	float NextRobTime = 0.0f;
	void ApplyDefaultTexts();
};
