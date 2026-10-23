#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GCTypes.h"
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Real Estate|Listing")
    FString StreetAddress = TEXT("42 Pacific View Drive");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Real Estate|Listing")
    EGCPropertyType PropertyType = EGCPropertyType::SingleFamilyHome;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Real Estate|Listing", meta = (ClampMin = "0"))
    int32 Bedrooms = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Real Estate|Listing", meta = (ClampMin = "0"))
    int32 Bathrooms = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Real Estate|Listing", meta = (ClampMin = "0"))
    int32 FloorAreaSquareFeet = 2200;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Real Estate|Listing", meta = (ClampMin = "0"))
    int64 AssessedValue = 425000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Real Estate|Listing", meta = (ClampMin = "0"))
    int64 AnnualPropertyTax = 5100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Real Estate|Listing", meta = (ClampMin = "0"))
    int64 MonthlyMaintenance = 260;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Real Estate|Listing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float InitialCondition = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Real Estate|Listing", meta = (ClampMin = "1000"))
	int32 PurchasePrice = 125000;

	/** Assign a pedestrian realtor in the level. Normal game damage can kill them; claiming the home then counts as burglary. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Real Estate|Listing")
	AGCPedestrian* Realtor = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Real Estate")
	void Interact(AGCCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Gridlock|Property")
    void SellOwnedProperty(AGCCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Gridlock|Property")
    void PayPropertyTax(AGCCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Gridlock|Property")
    void MaintainOwnedProperty(AGCCharacter* Character);

protected:
	UFUNCTION()
	void OnTriggerBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);

	UFUNCTION()
	void OnTriggerEnd(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
