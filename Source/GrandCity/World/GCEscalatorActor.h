#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCEscalatorActor.generated.h"

class ACharacter;
class UBoxComponent;
class USceneComponent;
class USplineComponent;
class UStaticMesh;
class UInstancedStaticMeshComponent;
class UPrimitiveComponent;

/** Functional escalator prototype: circulating treads and a ride zone for walking characters. */
UCLASS(Blueprintable)
class GRANDCITY_API AGCEscalatorActor : public AActor
{
	GENERATED_BODY()

public:
	AGCEscalatorActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Escalator")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Escalator")
	USplineComponent* TreadPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Escalator")
	UInstancedStaticMeshComponent* Treads;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Escalator")
	UInstancedStaticMeshComponent* TreadDetailVisuals;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Escalator")
	UBoxComponent* RideTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escalator|Shape", meta = (ClampMin = "200.0"))
	float RampRunCm = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escalator|Shape", meta = (ClampMin = "100.0"))
	float RampRiseCm = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escalator|Shape", meta = (ClampMin = "80.0"))
	float WidthCm = 190.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escalator|Motion", meta = (ClampMin = "8.0", ClampMax = "80.0"))
	float StepSpeedCmPerSecond = 42.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escalator|Motion", meta = (ClampMin = "8", ClampMax = "64"))
	int32 StepCount = 28;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escalator|Geometry")
	TObjectPtr<UStaticMesh> TreadMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escalator|Geometry")
	TObjectPtr<UStaticMesh> TreadDetailMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escalator|Geometry")
	FVector TreadDimensionsCm = FVector(70.0f, 170.0f, 18.0f);

protected:
	virtual void BeginPlay() override;

private:
	struct FRiderState
	{
		TWeakObjectPtr<ACharacter> Character;
		uint8 PreviousMovementMode = 0;
		uint8 PreviousCustomMovementMode = 0;
		float PreviousMaxFlySpeed = 0.0f;
	};

	float TreadPhaseCm = 0.0f;
	TArray<FRiderState> Riders;

	void RebuildPathAndTreads();
	void UpdateTreadTransforms();
	void RestoreRider(int32 Index);

	UFUNCTION()
	void OnRideBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRideEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);
};
