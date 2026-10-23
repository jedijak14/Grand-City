#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GCPhoneWidget.generated.h"

class UBorder;
class UTextBlock;

/** Slide-in in-game phone shell for wallet and travel-ticket information. */
UCLASS()
class GRANDCITY_API UGCPhoneWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPhoneOpen(bool bOpen);
	void SetPhoneData(int32 WalletBalance, const FString& TravelSummary);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBorder> PhoneFrame;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WalletText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TravelText;

	FString CachedTravelSummary;
	int32 CachedWalletBalance = 0;
	float OpenProgress = 0.0f;
	bool bWantsOpen = false;

	void RefreshPhoneText();
};
