#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/GCFlightTypes.h"
#include "GCComputerTerminalActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USceneComponent;
class UGCVoiceLineComponent;
class AGCCharacter;

/**
 * Airport computer / check-in desk.
 *
 * Flow (all in C++, screen drawn by the HUD today, UMG later):
 *   Walk up -> check-in agent greets you (voice line) -> [Tab] pick flight,
 *   [B] pick class, [E] book  ->  [E] again to check in (seat + gate assigned,
 *   boarding pass in your PlayerState wallet)  ->  walk to the airliner, [F] board.
 * Staff voice = GCVoiceLineComponent ("CheckInGreeting", "CheckInComplete", ...).
 * Place a BP pedestrian behind the desk at StaffSeat for a visible agent.
 */
UCLASS(Blueprintable)
class GRANDCITY_API AGCComputerTerminalActor : public AActor
{
	GENERATED_BODY()

public:
	AGCComputerTerminalActor();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Computer")
	UBoxComponent* Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Computer")
	UStaticMeshComponent* DeskMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Computer")
	UStaticMeshComponent* ScreenMesh;

	/** Where to stand your staff pedestrian Blueprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Computer")
	USceneComponent* StaffSeat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Computer")
	UGCVoiceLineComponent* Voice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Computer|Airport")
	FString AirportCode = TEXT("GCI");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Computer|Airport")
	FString AirportCity = TEXT("Gridlock International");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Computer|Airport")
	FString DestinationCode = TEXT("NIS");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Computer|Airport")
	FString DestinationCity = TEXT("North Island");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Computer|Airport")
	FString AirlineName = TEXT("Gridline Air");

	/** Leave empty to auto-generate a small schedule in BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Computer|Flights")
	TArray<FGCFlightOffer> Offers;

	UPROPERTY(BlueprintReadOnly, Category = "Computer|Flights")
	int32 SelectedOfferIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Computer|Flights")
	EGCSeatClass SelectedClass = EGCSeatClass::Economy;

	UFUNCTION(BlueprintCallable, Category = "Computer")
	void UseComputer(AGCCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Computer")
	void CycleOffer();

	UFUNCTION(BlueprintCallable, Category = "Computer")
	void CycleClass();

	UFUNCTION(BlueprintCallable, Category = "Computer")
	int32 PriceForSelected() const;

	/** Full text of the kiosk screen for HUD/UMG display. */
	UFUNCTION(BlueprintCallable, Category = "Computer")
	FString GetScreenText(AGCCharacter* Character) const;

	UFUNCTION(BlueprintCallable, Category = "Computer")
	bool BookFlight(AGCCharacter* Character, FGCFlightTicket& OutTicket);

	UFUNCTION(BlueprintCallable, Category = "Computer")
	bool CheckIn(AGCCharacter* Character);

protected:
	UFUNCTION()
	void OnTriggerBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);

	UFUNCTION()
	void OnTriggerEnd(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	void GenerateDefaultOffers();
	void Speak(AGCCharacter* Character, FName LineID, const FString& Fallback);
	static FString ClassName(EGCSeatClass SeatClass);
	static FString HourToText(float Hour);
};
