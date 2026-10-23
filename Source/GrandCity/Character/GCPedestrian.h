#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GCPedestrian.generated.h"

class UGCHealthComponent;
class UGCWeaponComponent;
class UGCVoiceLineComponent;
class USoundBase;
class UNiagaraSystem;
class AGCCharacter;

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

	UPROPERTY(BlueprintReadOnly, Category = "Pedestrian|Wallet")
	bool bHasBeenPickpocketed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Pedestrian|State")
	bool bKnockedDown = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pedestrian|Props")
	bool bHoldingCoffee = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pedestrian|Audio")
	USoundBase* CoffeeSpillSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pedestrian|FX")
	UNiagaraSystem* CoffeeSpillFX = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Pedestrian")
	void FleeFrom(const FVector& DangerLocation);

	/** Plays a voice line ("Ambient*" for civilians, "Police*" for officers), subtitling the player if close. */
	UFUNCTION(BlueprintCallable, Category = "Pedestrian")
	void SayLine(FName LineID);

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

	UFUNCTION(BlueprintCallable, Category = "Pedestrian|Reaction")
	void KnockDown(float DurationSeconds = 2.5f);

	void StartRagdoll();

protected:
	UFUNCTION()
	void OnDeath(AActor* Killer);

private:
	float NextAmbientVoiceTime = 0.0f;
	FTimerHandle KnockdownTimer;
	void RecoverFromKnockdown();
	void ShowSubtitleToNearbyPlayer(const FString& Text) const;
};
