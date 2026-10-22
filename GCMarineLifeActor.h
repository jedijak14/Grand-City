#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCMarineLifeActor.generated.h"

class USceneComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

/**
 * Lightweight marine life around a marina/reef point:
 * a circling fish school (instanced, per-fish phase) and leaping dolphins,
 * all riding the same ocean wave field as boats/surf. Assign real fish/dolphin
 * meshes in a BP child; fallback shapes are stretched engine spheres.
 */
UCLASS(Blueprintable)
class GRANDCITY_API AGCMarineLifeActor : public AActor
{
	GENERATED_BODY()

public:
	AGCMarineLifeActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Marine")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Marine")
	UInstancedStaticMeshComponent* Fish;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Marine")
	TArray<UStaticMeshComponent*> Dolphins;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marine")
	int32 FishCount = 80;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marine")
	float SchoolRadius = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marine")
	float SwimDepthCm = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marine")
	float WaterBaseLevelZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marine")
	FVector FishInstanceScale = FVector(0.55f, 0.16f, 0.16f);

private:
	struct FFishState
	{
		float OrbitRadius = 0.0f;
		float OrbitPhase = 0.0f;
		float OrbitSpeed = 0.0f;
		float DepthOffset = 0.0f;
		float WigglePhase = 0.0f;
		FVector LastPosition = FVector::ZeroVector;
	};

	TArray<FFishState> FishStates;
	TArray<float> DolphinPhases;
	TArray<FVector> DolphinLastPositions;
};
