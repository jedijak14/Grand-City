#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "GCDeformationComponent.generated.h"

class UStaticMeshComponent;

/**
 * Crash deformation ("soft-body style" denting).
 *
 * Honest note: Unreal has no built-in BeamNG-class soft-body vehicle solver.
 * This component gives the playable equivalent used by many shipped games:
 * the body mesh is copied into a procedural mesh, and every impact permanently
 * dents vertices near the hit point (radius + depth scaled by impact energy),
 * with cumulative damage driving smoke/fire/drivability in the vehicle pawn.
 * Requires the source static mesh to have "Allow CPU Access" enabled on import.
 */
UCLASS(ClassGroup = (GrandCity), meta = (BlueprintSpawnableComponent))
class GRANDCITY_API UGCDeformationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGCDeformationComponent();

	/** Max distance (cm) a single vertex may travel from its original position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deformation")
	float MaxVertexDisplacement = 45.0f;

	/** 0 = pristine, 1 = wrecked. */
	UPROPERTY(BlueprintReadOnly, Category = "Deformation")
	float DamageLevel = 0.0f;

	/**
	 * Copy Source into TargetProcMesh and hide Source. Call in BeginPlay of the vehicle.
	 * Returns false if the mesh has no CPU-accessible geometry (vehicle then just uses the static mesh).
	 */
	UFUNCTION(BlueprintCallable, Category = "Deformation")
	bool InitializeFromStaticMesh(UStaticMeshComponent* Source, UProceduralMeshComponent* TargetProcMesh);

	/** Dent the body. Points/normal are in the procedural mesh component's local space. */
	UFUNCTION(BlueprintCallable, Category = "Deformation")
	void ApplyDentLocal(const FVector& LocalPoint, const FVector& LocalImpactDirection, float Radius, float Depth);

	/** Convenience world-space version. ImpactDirection points INTO the body. */
	UFUNCTION(BlueprintCallable, Category = "Deformation")
	void ApplyDentWorld(const FVector& WorldPoint, const FVector& WorldImpactDirection, float Radius, float Depth);

	UFUNCTION(BlueprintCallable, Category = "Deformation")
	void RepairAll();

	UFUNCTION(BlueprintCallable, Category = "Deformation")
	bool IsInitialized() const { return bInitialized; }

private:
	UPROPERTY()
	UProceduralMeshComponent* ProcMesh = nullptr;

	bool bInitialized = false;
	TArray<FVector> OriginalPositions;
	TArray<FVector> CurrentPositions;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	TArray<uint32> Indices;

	void RefreshMeshSection();
};
