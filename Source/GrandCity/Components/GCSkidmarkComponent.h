#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "GCSkidmarkComponent.generated.h"

/**
 * World-space skidmark ribbon builder. One per vehicle.
 * Feed it contact points while a tyre is slipping; it stitches quads on the ground.
 * Needs a translucent vertex-colour material (see README material setup).
 */
UCLASS(ClassGroup = (GrandCity), meta = (BlueprintSpawnableComponent))
class GRANDCITY_API UGCSkidmarkComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:
	UGCSkidmarkComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skidmarks")
	int32 MaxVertices = 60000;

	/** Minimum travel (cm) between ribbon segments. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skidmarks")
	float MinSegmentLength = 18.0f;

	UFUNCTION(BlueprintCallable, Category = "Skidmarks")
	void AddSkidPoint(int32 WheelId, const FVector& WorldLocation, const FVector& GroundNormal, float Width, float Alpha);

	/** Break the ribbon for a wheel (tyre regained grip / left the ground). */
	UFUNCTION(BlueprintCallable, Category = "Skidmarks")
	void EndSkid(int32 WheelId);

	UFUNCTION(BlueprintCallable, Category = "Skidmarks")
	void ClearAll();

private:
	struct FLastPoint
	{
		FVector Center = FVector::ZeroVector;
		FVector Right = FVector::RightVector;
		bool bValid = false;
	};

	TMap<int32, FLastPoint> LastPoints;
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<int32> Triangles;
	bool bSectionCreated = false;

	void RebuildSection();
};
