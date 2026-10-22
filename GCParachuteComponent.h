#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GCParachuteComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGCOnParachuteDeployed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGCOnParachuteLanded);

/**
 * Parachute simulation for a Character: capped descent, forward glide,
 * steer input, flare (pull both toggles near ground = walk backwards input),
 * and a sway phase the canopy mesh/animation can read for cloth-like motion.
 */
UCLASS(ClassGroup = (GrandCity), meta = (BlueprintSpawnableComponent))
class GRANDCITY_API UGCParachuteComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGCParachuteComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadOnly, Category = "Parachute")
	bool bDeployed = false;

	/** Terminal descent speed under canopy (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parachute")
	float DescentSpeed = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parachute")
	float ForwardGlideSpeed = 650.0f;

	/** Minimum fall speed before deployment is allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parachute")
	float MinDeployFallSpeed = 250.0f;

	/** Reads sin/cos sway for canopy cloth motion in the owning character. */
	UPROPERTY(BlueprintReadOnly, Category = "Parachute")
	float SwayPhase = 0.0f;

	UPROPERTY(BlueprintAssignable, Category = "Parachute")
	FGCOnParachuteDeployed OnDeployed;

	UPROPERTY(BlueprintAssignable, Category = "Parachute")
	FGCOnParachuteLanded OnLanded;

	UFUNCTION(BlueprintCallable, Category = "Parachute")
	bool TryDeploy();

	UFUNCTION(BlueprintCallable, Category = "Parachute")
	void CutAway();

	UFUNCTION(BlueprintCallable, Category = "Parachute")
	void SetSteerInput(const FVector2D& InSteer) { SteerInput = InSteer; }

private:
	FVector2D SteerInput = FVector2D::ZeroVector;
};
