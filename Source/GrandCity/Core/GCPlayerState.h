#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Core/GCTypes.h"
#include "World/GCFlightTypes.h"
#include "GCPlayerState.generated.h"

class AGCVehiclePawn;

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

    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Identity")
    FDateTime CitizenDateOfBirth;

    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Identity")
    EGCCitizenSex CitizenSex = EGCCitizenSex::Unspecified;

    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Identity")
    FString DriverLicenseType = TEXT("Class C");

    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Identity")
    FString DriverLicenseNumber;

    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Identity")
    FDateTime LicenseIssuedUtc;

    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Identity")
    FDateTime LicenseExpiryUtc;

    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Identity")
    FDateTime CitizenCreatedUtc;

    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Identity")
    FString CurrentAddress;

    UPROPERTY(BlueprintReadWrite, Category = "Gridlock|Appearance")
    FName CurrentOutfitID;

    /** Skill range 0-1 used by stealth crime success and awareness checks. */
    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Skills", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StealthSkill = 0.15f;

	UPROPERTY(BlueprintReadWrite, Category = "Grand City")
	EGCJob CurrentJob = EGCJob::Civilian;

    UPROPERTY(BlueprintReadOnly, Category = "Police|Career")
    int32 ConductPoints = 100;

    UPROPERTY(BlueprintReadOnly, Category = "Police|Career")
    int32 ShiftPoints = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Police|Career")
    bool bPoliceShiftActive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Police|Career")
    bool bPoliceShiftFailed = false;

    UPROPERTY(BlueprintReadOnly, Category = "Police|Equipment")
    bool bHasAlcoholTestKit = false;

    UPROPERTY(BlueprintReadOnly, Category = "Police|Equipment")
    bool bHasDrugTestKit = false;

    UPROPERTY(BlueprintReadOnly, Category = "Police|Identity")
    FString PoliceBadgeNumber;

    UPROPERTY(BlueprintReadOnly, Category = "Police|Records")
    TArray<FGCPoliceCitationRecord> PoliceCitationRecords;

	UPROPERTY(BlueprintReadOnly, Category = "Grand City|Homes")
	TArray<FString> OwnedPropertyIDs;

	UPROPERTY(BlueprintReadOnly, Category = "Grand City|Homes")
	TArray<FString> StolenPropertyIDs;

    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Property")
    TArray<FGCPropertyRecord> PropertyPortfolio;

    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Property")
    FString PrimaryResidenceID;

    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Vehicle Ownership")
    TArray<FGCOwnedVehicleRecord> OwnedVehicleRecords;

    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Progression")
    double TotalPlaytimeSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category = "Gridlock|Progression")
    int64 LastProcessedGameDay = 0;

    UPROPERTY(Transient)
    FString LastSavedMapName;

    UPROPERTY(Transient)
    int64 LastSavedGameDayIndex = 0;

    UPROPERTY(Transient)
    float LastSavedTimeOfDay = 10.0f;

    UPROPERTY(Transient)
    FTransform LastSavedPlayerTransform = FTransform::Identity;

    UPROPERTY(Transient)
    bool bHasSavedReturnLocation = false;

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

    UFUNCTION(BlueprintCallable, Category = "Gridlock|Identity")
    bool CreateCitizenProfile(const FString& DisplayName, const FDateTime& DateOfBirth,
        EGCCitizenSex Sex, const FString& LicenseType);

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
	void SetTicket(const FGCFlightTicket& NewTicket);

	UFUNCTION(BlueprintCallable, Category = "Grand City|Flights")
	void ClearTicket();

	UFUNCTION(BlueprintCallable, Category = "Grand City|Homes")
	bool OwnsProperty(const FString& PropertyID) const;

	UFUNCTION(BlueprintCallable, Category = "Grand City|Homes")
	bool AddOwnedProperty(const FString& PropertyID, bool bStolen);

    UFUNCTION(BlueprintCallable, Category = "Gridlock|Property")
    bool AcquireProperty(const FGCPropertyRecord& Property);

    UFUNCTION(BlueprintCallable, Category = "Gridlock|Property")
    bool SetPrimaryResidence(const FString& PropertyID);

    UFUNCTION(BlueprintCallable, Category = "Gridlock|Property")
    int64 GetPropertySaleOffer(const FString& PropertyID) const;

    UFUNCTION(BlueprintCallable, Category = "Gridlock|Property")
    bool SellProperty(const FString& PropertyID);

    UFUNCTION(BlueprintCallable, Category = "Gridlock|Property")
    bool PayPropertyTax(const FString& PropertyID);

    UFUNCTION(BlueprintCallable, Category = "Gridlock|Property")
    bool PayPropertyMaintenance(const FString& PropertyID);

    UFUNCTION(BlueprintCallable, Category = "Gridlock|Property")
    bool ProcessPropertyDay(int64 NewGameDayIndex);

    void RegisterOwnedVehicle(AGCVehiclePawn* Vehicle, bool bStolen);
    void SyncOwnedVehicles();

    UFUNCTION(BlueprintCallable, Category = "Gridlock|Save")
    void RequestPersistentSave();

    UFUNCTION(BlueprintCallable, Category = "Police|Career")
    void StartPoliceShift();

    UFUNCTION(BlueprintCallable, Category = "Police|Career")
    bool IsPoliceOnDuty() const;

    UFUNCTION(BlueprintCallable, Category = "Police|Career")
    void ApplyConductPenalty(int32 Points);

    UFUNCTION(BlueprintCallable, Category = "Police|Career")
    void AwardShiftPoints(int32 Points);

    UFUNCTION(BlueprintCallable, Category = "Police|Records")
    void RecordPoliceCitation(const FGCPoliceCitationRecord& Citation);

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
