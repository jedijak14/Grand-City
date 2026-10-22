#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "GCDeformationComponent.generated.h"

class UStaticMeshComponent;

/** CPU-side data for one material section of the copied vehicle body. */
struct FGCDeformationMeshSection
{
	int32 SectionIndex = INDEX_NONE;
	TArray<FVector> OriginalPositions;
	TArray<FVector> CurrentPositions;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	TArray<int32> Indices;
};

/**
 * Crash deformation using persistent, surface-aware vertex dents.
 *
 * This is a visual deformation approximation, not a BeamNG-class node/beam soft-body solver.
 * The source static mesh must have CPU access enabled. All material sections are copied and
 * deformed independently so multi-material vehicle bodies remain intact.
 */
UCLASS(ClassGroup = (GrandCity), meta = (BlueprintSpawnableComponent))
class GRANDCITY_API UGCDeformationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGCDeformationComponent();

	/** Maximum cumulative displacement (cm) of any vertex from its original position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deformation", meta = (ClampMin = "1.0"))
	float MaxVertexDisplacement = 45.0f;

	/** 0 = pristine, 1 = heavily damaged. */
	UPROPERTY(BlueprintReadOnly, Category = "Deformation")
	float DamageLevel = 0.0f;

	/** Copies every LOD0 material section into TargetProcMesh. */
	UFUNCTION(BlueprintCallable, Category = "Deformation")
	bool InitializeFromStaticMesh(UStaticMeshComponent* Source, UProceduralMeshComponent* TargetProcMesh);

	/** Dent the body. Point and direction are in procedural-mesh local space. */
	UFUNCTION(BlueprintCallable, Category = "Deformation")
	void ApplyDentLocal(const FVector& LocalPoint, const FVector& LocalImpactDirection, float Radius, float Depth);

	/** Convenience world-space version. ImpactDirection points into the body. */
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
	TArray<FGCDeformationMeshSection> MeshSections;

	void RefreshMeshSection(FGCDeformationMeshSection& Section);
};
