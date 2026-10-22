#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCGrassFieldActor.generated.h"

class USceneComponent;
class UHierarchicalInstancedStaticMeshComponent;

/**
 * Scatter-placed instanced grass. Author the wind in the grass MATERIAL using
 * PerInstanceCustomData[0] as a per-blade phase (see Docs/PHASE2_VISUALS.md):
 * WPO.x += sin(Time*2 + phase*6.28) * WindStrength * UV.y (or vertex color mask).
 * Density honors the settings toggle at BeginPlay; call Regenerate() after changes.
 */
UCLASS(Blueprintable)
class GRANDCITY_API AGCGrassFieldActor : public AActor
{
	GENERATED_BODY()

public:
	AGCGrassFieldActor();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grass")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grass")
	UHierarchicalInstancedStaticMeshComponent* Grass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass")
	FVector2D FieldSize = FVector2D(5200.0f, 5200.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass", meta = (ClampMin = "0", ClampMax = "30000"))
	int32 InstanceCount = 5200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass")
	int32 RandomSeed = 2048;

	/** Fallback engine cube is 100cm: default scale makes slim blades. For a real
	 *  grass mesh authored at true size, set this to (1,1,1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass")
	FVector BaseInstanceScale = FVector(0.05f, 0.05f, 0.55f);

	UFUNCTION(BlueprintCallable, Category = "Grass")
	void Regenerate();
};
