#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Core/GCTypes.h"
#include "World/GCFlightTypes.h"
#include "GCPlayerState.generated.h"

/** Money, wanted stars and current job for a player. */
UCLASS()
class GRANDCITY_API AGCPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AGCPlayerState();

	UPROPERTY(BlueprintReadWrite, Category = "Grand City")
	int32 Money = 1500;

	UPROPERTY(BlueprintReadOnly, Category = "Grand City|Wanted")
	int32 WantedStars = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Grand City")
	EGCJob CurrentJob = EGCJob::Civilian;

	/** Time remaining before one wanted star decays while hidden from police. */
	UPROPERTY(EditDefaultsOnly, Category = "Grand City|Wanted")
	float WantedDecaySeconds = 35.0f;

	UFUNCTION(BlueprintCallable, Category = "Grand City")
	void AddMoney(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Grand City")
	bool SpendMoney(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Grand City|Wanted")
	void AddWantedStars(int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Grand City|Wanted")
	void ClearWanted();

	/** Call every frame while no police unit can see the player. */
	UFUNCTION(BlueprintCallable, Category = "Grand City|Wanted")
	void TickWantedDecay(float DeltaSeconds, bool bSeenByPolice);

	// ------------------------------------------------ flight ticket wallet
	UPROPERTY(BlueprintReadOnly, Category = "Grand City|Flights")
	FGCFlightTicket ActiveTicket;

	UFUNCTION(BlueprintCallable, Category = "Grand City|Flights")
	bool HasActiveTicket() const { return ActiveTicket.Status != EGCTicketStatus::None && ActiveTicket.Status != EGCTicketStatus::Cancelled; }

	UFUNCTION(BlueprintCallable, Category = "Grand City|Flights")
	void SetTicket(const FGCFlightTicket& NewTicket) { ActiveTicket = NewTicket; }

	UFUNCTION(BlueprintCallable, Category = "Grand City|Flights")
	void ClearTicket() { ActiveTicket = FGCFlightTicket(); }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWantedChanged, int32, NewStars);
	UPROPERTY(BlueprintAssignable, Category = "Grand City|Wanted")
	FOnWantedChanged OnWantedChanged;

private:
	float HiddenTime = 0.0f;
};
