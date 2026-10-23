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

	/** Physical cash on hand. */
	UPROPERTY(BlueprintReadWrite, Category = "Grand City|Wallet")
	int32 Money = 1500;

	/** Shared linked-account balance used by fictional NFC wallet providers. */
	UPROPERTY(BlueprintReadWrite, Category = "Grand City|Wallet")
	int32 DigitalBalance = 2500;

	UPROPERTY(BlueprintReadWrite, Category = "Gridlock|Wallet")
	EGCPaymentMethod SelectedPaymentMethod = EGCPaymentMethod::Cash;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gridlock|Wallet")
	int32 CreditLimit = 5000;

	UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Wallet")
	int32 CreditUsed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Grand City|Wanted")
	int32 WantedStars = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Gridlock|Identity")
	FString CitizenName;

	UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Identity")
	FString CitizenID;

	UPROPERTY(BlueprintReadWrite, Category = "Grand City")
	EGCJob CurrentJob = EGCJob::Civilian;

	UPROPERTY(BlueprintReadOnly, Category = "Grand City|Homes")
	TArray<FString> OwnedPropertyIDs;

	UPROPERTY(BlueprintReadOnly, Category = "Grand City|Homes")
	TArray<FString> StolenPropertyIDs;

	UPROPERTY(BlueprintReadOnly, Category = "Grand City|Devices")
	TArray<FString> OwnedDeviceIDs;

	UPROPERTY(BlueprintReadWrite, Category = "Grand City|Devices")
	FString ActivePhoneModel;

	UPROPERTY(BlueprintReadWrite, Category = "Grand City|Devices")
	FString ActivePhoneOS;

	/** Time remaining before one wanted star decays while hidden from police. */
	UPROPERTY(EditDefaultsOnly, Category = "Grand City|Wanted")
	float WantedDecaySeconds = 35.0f;

	UFUNCTION(BlueprintCallable, Category = "Grand City")
	void AddMoney(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Grand City|Wallet")
	bool SpendMoney(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Grand City|Wallet")
	void RefundMoney(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Gridlock|Wallet")
	bool RepayCreditCard(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Grand City|Wallet")
	void CyclePaymentMethod();

	UFUNCTION(BlueprintCallable, Category = "Grand City|Wallet")
	FString GetPaymentMethodLabel() const;

	UFUNCTION(BlueprintCallable, Category = "Grand City|Jobs")
	int32 TickJobIncome(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Gridlock|Identity")
	bool CreateCitizenIdentity(const FString& DisplayName);

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

	UFUNCTION(BlueprintCallable, Category = "Grand City|Homes")
	bool OwnsProperty(const FString& PropertyID) const;

	UFUNCTION(BlueprintCallable, Category = "Grand City|Homes")
	bool AddOwnedProperty(const FString& PropertyID, bool bStolen);

	UFUNCTION(BlueprintCallable, Category = "Grand City|Devices")
	bool OwnsDevice(const FString& DeviceID) const;

	UFUNCTION(BlueprintCallable, Category = "Grand City|Devices")
	void AddOwnedDevice(const FString& DeviceID, bool bPhone, const FString& PhoneModel, const FString& PhoneOS);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWantedChanged, int32, NewStars);
	UPROPERTY(BlueprintAssignable, Category = "Grand City|Wanted")
	FOnWantedChanged OnWantedChanged;

private:
	float HiddenTime = 0.0f;
	float JobPayTimer = 0.0f;
	EGCJob LastPaidJob = EGCJob::Civilian;
};
