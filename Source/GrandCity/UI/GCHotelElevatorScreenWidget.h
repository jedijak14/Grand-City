#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GCHotelElevatorScreenWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UTexture2D;

/** Native world-space hotel elevator display with a live floor readout and rotating hotel campaigns. */
UCLASS()
class GRANDCITY_API UGCHotelElevatorScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGCHotelElevatorScreenWidget(const FObjectInitializer& ObjectInitializer);

	void SetElevatorReadout(int32 CurrentFloor, bool bMoving, int32 DestinationFloor);
	void AnimateFloorSelection(int32 CurrentFloor, int32 DestinationFloor);
	void SetIdleFloorButtonLabel(int32 CurrentFloor);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UImage> HotelAdImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FloorReadout;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HotelFallback;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> FloorButtonFrames;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> FloorButtonLabels;

	UPROPERTY(EditDefaultsOnly, Category = "Elevator Screen|Hotel Ads")
	TArray<TSoftObjectPtr<UTexture2D>> HotelAdAssets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> HotelAdSlides;

	int32 ActiveAdIndex = INDEX_NONE;
	int32 PressedFloorButtonIndex = INDEX_NONE;
	int32 CachedFloorButtonCurrent = MIN_int32;
	int32 CachedFloorButtonDestination = MIN_int32;
	float ButtonPressRemaining = 0.0f;
	int32 CachedCurrentFloor = MIN_int32;
	int32 CachedDestinationFloor = MIN_int32;
	bool bCachedMoving = false;
	float AdSlideTime = 0.0f;

	void AdvanceAdSlide();
	void RefreshFloorReadout();
};
