#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GCPedestrian.generated.h"

class UGCHealthComponent;
class UGCWeaponComponent;
class UGCVoiceLineComponent;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pedestrian")
	bool bIsPolice = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pedestrian")
	float WalkSpeed = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pedestrian")
	float RunSpeed = 560.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Pedestrian")
	bool bDead = false;

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

	void StartRagdoll();

protected:
	UFUNCTION()
	void OnDeath(AActor* Killer);

private:
	float NextAmbientVoiceTime = 0.0f;
	void ShowSubtitleToNearbyPlayer(const FString& Text) const;
};
