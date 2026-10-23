#pragma once

#include "CoreMinimal.h"
#include "Core/GCTypes.h"
#include "GameFramework/Character.h"
#include "GCPedestrian.generated.h"

class UGCHealthComponent;
class UGCWeaponComponent;
class UGCVoiceLineComponent;
class UAudioComponent;
class USoundBase;
class UNiagaraSystem;
class AGCCharacter;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EGCHandheldItem : uint8
{
    None,
    Coffee,
    Phone,
    Bag
};

/** Civilian or on-foot police NPC. Behaviour lives in AGCPedAIController. */
UCLASS(Blueprintable)
class GRANDCITY_API AGCPedestrian : public ACharacter
{
	GENERATED_BODY()

public:
	AGCPedestrian();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pedestrian")
	UGCHealthComponent* Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pedestrian")
	UGCWeaponComponent* Weapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pedestrian")
	UGCVoiceLineComponent* Voice;

	/** Assign a suitable nonverbal panic clip in the pedestrian Blueprint; voice line subtitles remain the fallback. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pedestrian|Audio")
	USoundBase* PanicScreamSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pedestrian")
	bool bIsPolice = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pedestrian")
	float WalkSpeed = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pedestrian")
	float RunSpeed = 560.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Pedestrian")
	bool bDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pedestrian|Wallet", meta = (ClampMin = "0"))
	int32 WalletCash = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Identity")
    FString CitizenID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Identity")
    bool bIDIsFake = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Identity")
    FString CitizenName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Identity")
    FDateTime CitizenDateOfBirth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Identity")
    EGCCitizenSex CitizenSex = EGCCitizenSex::Unspecified;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Identity")
    FString DriverLicenseType = TEXT("Class C");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Identity")
    FString DriverLicenseNumber;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Identity")
    FDateTime DriverLicenseExpiryUtc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Records")
    bool bHasOpenWarrant = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Records")
    bool bHasPriorCriminalRecord = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Records")
    FString PriorCriminalRecordSummary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Search")
    bool bHasWeapon = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Search")
    bool bHasContraband = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Search")
    bool bNervous = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Search")
    bool bSmellsOfCannabis = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Search")
    bool bDilatedPupils = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Intoxication")
    bool bIntoxicatedByAlcohol = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Intoxication")
    bool bIntoxicatedByDrugs = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Intoxication")
    bool bSlurredSpeech = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Intoxication")
    bool bUnsteadyGait = false;

    UPROPERTY(BlueprintReadOnly, Category = "Citizen|Intoxication")
    bool bDUIConfirmed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Observed Violations")
    bool bObservedJaywalking = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Observed Violations")
    bool bObservedLittering = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen|Observed Violations")
    bool bObservedLoitering = false;

    UPROPERTY(BlueprintReadOnly, Category = "Citizen|Police")
    bool bStoppedForPolice = false;

    UPROPERTY(BlueprintReadOnly, Category = "Citizen|Police")
    bool bArrested = false;

    UPROPERTY(BlueprintReadOnly, Category = "Citizen|Witness")
    FString LastWitnessStatement;

    UPROPERTY(BlueprintReadOnly, Category = "Citizen|Incident")
    bool bWasVehicleCrashVictim = false;

    UPROPERTY(BlueprintReadOnly, Category = "Citizen|Incident")
    FVector LastIncidentLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Pedestrian|Wallet")
	bool bHasBeenPickpocketed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Pedestrian|State")
	bool bKnockedDown = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pedestrian|Props")
	bool bHoldingCoffee = false;

    UPROPERTY(BlueprintReadOnly, Category = "Pedestrian|Props")
    EGCHandheldItem HeldItem = EGCHandheldItem::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pedestrian|Props")
    TObjectPtr<UStaticMeshComponent> CarriedItemMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pedestrian|Props")
    TObjectPtr<UStaticMesh> CoffeeCupMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pedestrian|Props")
    TObjectPtr<UStaticMesh> PhonePropMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pedestrian|Props")
    TObjectPtr<UStaticMesh> HandbagMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pedestrian|FX")
    TObjectPtr<UMaterialInterface> CoffeeSpillStainMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pedestrian|Audio")
	USoundBase* CoffeeSpillSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pedestrian|FX")
	UNiagaraSystem* CoffeeSpillFX = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Pedestrian")
	void FleeFrom(const FVector& DangerLocation);

	/** Plays a voice line ("Ambient*" for civilians, "Police*" for officers), subtitling the player if close. */
	UFUNCTION(BlueprintCallable, Category = "Pedestrian")
	void SayLine(FName LineID);

    UFUNCTION(BlueprintCallable, Category = "Citizen|Interaction")
    void InteractWithPlayer(AGCCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Police|Interaction")
    bool RequestIdentification(AGCCharacter* Officer);
    bool InitiatePoliceStop(AGCCharacter* Officer);

    UFUNCTION(BlueprintCallable, Category = "Police|Interaction")
    bool Frisk(AGCCharacter* Officer);

    UFUNCTION(BlueprintCallable, Category = "Police|Interaction")
    bool SearchPersonAndBag(AGCCharacter* Officer);

    UFUNCTION(BlueprintCallable, Category = "Police|Interaction")
    bool Arrest(AGCCharacter* Officer);

    UFUNCTION(BlueprintCallable, Category = "Police|Interaction")
    bool ReleaseFromPoliceStop(AGCCharacter* Officer);

    UFUNCTION(BlueprintCallable, Category = "Police|Interaction")
    void InterviewAboutCrime(AGCCharacter* Officer);
    bool RunAlcoholTest(AGCCharacter* Officer);
    bool RunDrugTest(AGCCharacter* Officer);

    UFUNCTION(BlueprintCallable, Category = "Police|Interaction")
    bool IssueCitation(AGCCharacter* Officer, EGCPoliceViolation Violation);

	/** AI calls this periodically; internal cooldown + distance checks keep it natural. */
	UFUNCTION(BlueprintCallable, Category = "Pedestrian")
	void MaybeSayAmbient();

	/** Police only: fire one burst toward the target's chest. */
	UFUNCTION(BlueprintCallable, Category = "Pedestrian")
	void FireAtActor(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Pedestrian|Crime")
	bool AttemptPickpocket(AGCCharacter* Thief);

	UFUNCTION(BlueprintCallable, Category = "Pedestrian|Crime")
	bool CanWitnessCrimeAt(const FVector& IncidentLocation) const;

    void ReactToWitnessedCrime(const FVector& IncidentLocation, bool bScream);
    void RecordVehicleCrashWitness(const FVector& CrashLocation, bool bWasVictim, const FString& VehicleDescription);

    UFUNCTION(BlueprintCallable, Category = "Pedestrian|Crime")
    void QueuePoliceReport(int32 WantedStars, float DelaySeconds);

	UFUNCTION(BlueprintCallable, Category = "Pedestrian|Reaction")
	void KnockDown(float DurationSeconds = 2.5f);

	void StartRagdoll();

protected:
	UFUNCTION()
	void OnDeath(AActor* Killer);

private:
	float NextAmbientVoiceTime = 0.0f;
	FTimerHandle KnockdownTimer;
    FTimerHandle CrimeReportTimer;
    FTimerHandle PanicAudioOcclusionTimer;
    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> PanicScreamAudio = nullptr;
    int32 PendingWantedStars = 0;

    UFUNCTION()
    void CompletePoliceReport();

    UFUNCTION()
    void UpdatePanicAudioOcclusion();
	void RecoverFromKnockdown();
    void DropCarriedItem(bool bSpillCoffee);
    bool HasProbableCause() const;
    void StopForPolice();
	void ShowSubtitleToNearbyPlayer(const FString& Text) const;
};
