#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GCHotelElevatorScreenWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

/** Native world-space hotel elevator display with a live floor readout and rewards-partner carousel. */
UCLASS()
class GRANDCITY_API UGCHotelElevatorScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGCHotelElevatorScreenWidget(const FObjectInitializer& ObjectInitializer);

	void SetElevatorReadout(int32 CurrentFloor, bool bMoving, int32 DestinationFloor);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UImage> PartnerLogoImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FloorReadout;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PartnerName;

	UPROPERTY(EditDefaultsOnly, Category = "Elevator Screen|Rewards")
	TArray<TSoftObjectPtr<UTexture2D>> PartnerLogoAssets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> PartnerLogos;

	int32 ActivePartnerIndex = INDEX_NONE;
	int32 CachedCurrentFloor = MIN_int32;
	int32 CachedDestinationFloor = MIN_int32;
	bool bCachedMoving = false;
	float PartnerSlideTime = 0.0f;

	void AdvancePartnerSlide();
	void RefreshFloorReadout();
};
