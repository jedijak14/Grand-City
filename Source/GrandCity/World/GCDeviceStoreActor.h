#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCDeviceStoreActor.generated.h"

class AGCCharacter;
class UBoxComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EGCDeviceCategory : uint8
{
	Phone,
	Tablet,
	Desktop
};

USTRUCT(BlueprintType)
struct FGCDeviceOffer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DeviceID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString OperatingSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGCDeviceCategory Category = EGCDeviceCategory::Phone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 Price = 500;
};

/** Electronics retailer with fictional phone/tablet/desktop brands and OS names. */
UCLASS(Blueprintable)
class GRANDCITY_API AGCDeviceStoreActor : public AActor
{
	GENERATED_BODY()

public:
	AGCDeviceStoreActor();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Device Store")
	UBoxComponent* Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Device Store")
	UStaticMeshComponent* StoreSign;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Device Store|Catalog")
	TArray<FGCDeviceOffer> Offers;

	UPROPERTY(BlueprintReadOnly, Category = "Device Store|Catalog")
	int32 SelectedOfferIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "Device Store")
	void CycleOffer();

	UFUNCTION(BlueprintCallable, Category = "Device Store")
	void PurchaseSelected(AGCCharacter* Character);

protected:
	UFUNCTION()
	void OnTriggerBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);

	UFUNCTION()
	void OnTriggerEnd(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	TWeakObjectPtr<AGCCharacter> CurrentCustomer;
	void AddDefaultOffers();
	FString DescribeSelectedOffer() const;
};
