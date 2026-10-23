#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCRealEstateActor.generated.h"

class AGCCharacter;
class AGCPedestrian;
class UBoxComponent;
class UStaticMeshComponent;

/** Prototype residence listing: buy through a living agent or commit a wanted-level burglary after the agent is killed. */
UCLASS(Blueprintable)
class GRANDCITY_API AGCRealEstateActor : public AActor
{
	GENERATED_BODY()

public:
	AGCRealEstateActor();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Real Estate")
	UBoxComponent* Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Real Estate")
	UStaticMeshComponent* SignMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Real Estate|Listing")
	FString PropertyID = TEXT("GC-HOME-001");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Real Estate|Listing")
	FString PropertyName = TEXT("Pacific View House");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Real Estate|Listing", meta = (ClampMin = "1000"))
	int32 PurchasePrice = 125000;

	/** Assign a pedestrian realtor in the level. Normal game damage can kill them; claiming the home then counts as burglary. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Real Estate|Listing")
	AGCPedestrian* Realtor = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Real Estate")
	void Interact(AGCCharacter* Character);

protected:
	UFUNCTION()
	void OnTriggerBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);

	UFUNCTION()
	void OnTriggerEnd(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
