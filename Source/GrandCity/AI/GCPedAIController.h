#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GCPedAIController.generated.h"

class AGCPedestrian;

UENUM(BlueprintType)
enum class EGCPedState : uint8
{
	Wander,
	Flee,
	PoliceChase
};

/** Civilian wander/flee + on-foot police pursuit, arrest (low wanted) and shooting (3+ stars). */
UCLASS()
class GRANDCITY_API AGCPedAIController : public AAIController
{
	GENERATED_BODY()

public:
	AGCPedAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	EGCPedState State = EGCPedState::Wander;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float WanderRadius = 3500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Police")
	float ArrestRange = 190.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Police")
	float ArrestSeconds = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Police")
	float ShootRange = 4200.0f;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void StartFlee(const FVector& DangerLocation);

private:
	UPROPERTY()
	AGCPedestrian* Ped = nullptr;

	float DecisionTimer = 0.0f;
	float FleeUntilTime = 0.0f;
	float ArrestProgress = 0.0f;
	FVector FleeFromLocation = FVector::ZeroVector;

	void PickWanderTarget();
	void UpdateFlee();
	void UpdatePolice(float DeltaSeconds);
	void SetPedSpeed(float Speed);
	bool HasLineOfSightTo(AActor* Target) const;
	void BustedPlayer(APawn* PlayerPawn);
};
