#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "GCDeformationComponent.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UPrimitiveComponent;

/** CPU-side data for one material section of the copied vehicle body. */
struct FGCDeformationMeshSection
{
	int32 SectionIndex = INDEX_NONE;
	TArray<FVector> OriginalPositions;
	TArray<FVector> CurrentPositions;
	TArray<FVector> Normals;
    TArray<float> StructuralHardness;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	TArray<int32> Indices;
};

USTRUCT(BlueprintType)
struct FGCDetachableVehiclePart
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Damage")
    TObjectPtr<UStaticMeshComponent> Mesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Damage", meta = (ClampMin = "1.0"))
    float DetachStressThreshold = 320.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Damage")
    float AccumulatedStress = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Damage")
    bool bDetached = false;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> OriginalParent = nullptr;

    UPROPERTY(Transient)
    FTransform InitialRelativeTransform = FTransform::Identity;
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
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Maximum cumulative displacement (cm) of any vertex from its original position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deformation", meta = (ClampMin = "1.0"))
	float MaxVertexDisplacement = 45.0f;

    /** If enabled, imported vertex-color red (0 brittle, 255 hard) modulates dent depth. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deformation|Material")
    bool bUseVertexRedAsHardnessMask = false;

    /** Mesh pieces assigned in a vehicle Blueprint (hood, mirror, bumper, doors, trim). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deformation|Detachable Parts")
    TArray<FGCDetachableVehiclePart> DetachableParts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deformation|Detachable Parts", meta = (ClampMin = "1.0"))
    float StructuralStressPerDentCm = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deformation|Detachable Parts", meta = (ClampMin = "0.0"))
    float PartDetachImpulse = 1800.0f;

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

protected:
    UFUNCTION()
    void OnOwnerComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION()
    void OnOwnerComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY()
	UProceduralMeshComponent* ProcMesh = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UPrimitiveComponent> OwnerCollisionComponent = nullptr;

    void ProcessOwnerImpact(const FHitResult& Hit, const FVector& RelativeVelocity);

	bool bInitialized = false;
	TArray<FGCDeformationMeshSection> MeshSections;

	void RefreshMeshSection(FGCDeformationMeshSection& Section);
    void ApplyStructuralStress(const FVector& WorldPoint, const FVector& WorldDirection, float Radius, float Depth);
};
