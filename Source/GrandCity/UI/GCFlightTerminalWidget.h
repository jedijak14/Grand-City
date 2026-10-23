#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GCFlightTerminalWidget.generated.h"

class AGCCharacter;
class AGCComputerTerminalActor;
class UBorder;
class UTextBlock;

/** Airport kiosk overlay; keyboard selection drives an animated highlighted flight and action key. */
UCLASS()
class GRANDCITY_API UGCFlightTerminalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetTerminal(AGCComputerTerminalActor* InTerminal, AGCCharacter* InUser);
	void PlaySelectionFeedback();
	void PlayActionFeedback();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	TWeakObjectPtr<AGCComputerTerminalActor> Terminal;
	TWeakObjectPtr<AGCCharacter> User;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AirportTitle;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FlightScreenText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ActionLabel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> SelectionFrame;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ActionFrame;

	FString CachedScreenText;
	int32 CachedOfferIndex = INDEX_NONE;
	uint8 CachedSeatClass = 255;
	float SelectionPulseRemaining = 0.0f;
	float ActionPulseRemaining = 0.0f;

	void RefreshScreen(bool bCheckSelection);
};
