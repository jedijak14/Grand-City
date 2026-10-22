#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "GCOceanManager.generated.h"

class UStaticMeshComponent;

/** Everything a floating/planing/surfing body needs from one wave sample. */
USTRUCT(BlueprintType)
struct FGCOceanSample
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float Height = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	FVector Normal = FVector::UpVector;

	/** Vertical velocity of the surface itself (cm/s). */
	UPROPERTY(BlueprintReadOnly)
	float VerticalVelocity = 0.0f;

	/** Horizontal background ocean current (cm/s). */
	UPROPERTY(BlueprintReadOnly)
	FVector Current = FVector::ZeroVector;

	/** Combined horizontal current and wave-orbital velocity (cm/s). */
	UPROPERTY(BlueprintReadOnly)
	FVector SurfaceVelocity = FVector::ZeroVector;

	/** 0-1 foam/whitecap factor on crests (for materials & spray FX). */
	UPROPERTY(BlueprintReadOnly)
	float FoamFactor = 0.0f;
};

/** Short-lived, deterministic wake packet emitted by boats and low-flying aircraft. */
struct FGCWaterWakeImpulse
{
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D TravelDirection = FVector2D(1.0f, 0.0f);
	float StartTime = 0.0f;
	float StrengthCm = 0.0f;
	float LifetimeSeconds = 0.0f;
	float GrowthSpeedCmPerSecond = 0.0f;
	float WavelengthCm = 0.0f;
};

/**
 * Single source of truth for the analytic water field and bounded wake disturbances.
 * Boats, swimming and surfing query the same height field that the optional harbor mesh renders.
 */
UCLASS()
class GRANDCITY_API AGCOceanManager : public AActor
{
	GENERATED_BODY()

public:
	AGCOceanManager();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ocean")
	UStaticMeshComponent* WaterMesh;

	/** Optional CPU-deformed surface that uses the same analytic waves as buoyancy queries. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ocean|Visuals")
	UProceduralMeshComponent* WaveSurface;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Visuals")
	bool bGenerateVisualSurface = false;

	/** Half-size in centimetres; place the ocean actor so this surface covers the playable coastline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Visuals", meta = (ClampMin = "1000.0"))
	FVector2D VisualSurfaceHalfExtent = FVector2D(30000.0, 50000.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Visuals", meta = (ClampMin = "8", ClampMax = "256"))
	int32 VisualSurfaceGridResolution = 192;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Visuals", meta = (ClampMin = "0.016", ClampMax = "0.25"))
	float VisualSurfaceUpdateInterval = 0.033f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	float AmplitudeScale = 1.0f;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** World-space wave height (cm, absolute Z of the surface if the ocean actor sits at Z=BaseLevel). */
	UFUNCTION(BlueprintCallable, Category = "Ocean")
	float GetWaveHeightWorld(const FVector& WorldLocation) const;

	/** Pure wave function: XY in cm, time in seconds. Returns height offset in cm around 0. */
	static float GetWaveHeightAt(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale);

	/** Analytic wave-surface velocity (cm/s), including the background current. */
	static FVector GetSurfaceVelocityAt(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale);

	static FVector GetWaveNormalAt(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale);

	/** dHeight/dt at a point (cm/s) - boats use this to ride moving swells, not just heights. */
	static float GetWaveVerticalVelocityAt(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale);

	/** Slow tidal/wind surface current in cm/s (world XY). */
	static FVector GetCurrentAt(const FVector2D& XY, float TimeSeconds);

	/** 0-1 crest foam estimate (height + slope), for materials, spray and surf scoring. */
	static float GetFoamFactorAt(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale);

	/** One-call combined sample (cheaper than calling the pieces). */
	UFUNCTION(BlueprintCallable, Category = "Ocean")
	FGCOceanSample SampleOcean(const FVector& WorldLocation) const;

	/** Inject a short-lived V-shaped wake into both visual deformation and gameplay samples. */
	UFUNCTION(BlueprintCallable, Category = "Ocean|Wake")
	void EmitWake(const FVector& WorldLocation, const FVector& TravelDirection, float StrengthCm,
		float LifetimeSeconds = 4.0f, float GrowthSpeedCmPerSecond = 900.0f, float WavelengthCm = 900.0f);

	static FGCOceanSample SampleOceanStatic(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale);

private:
	void BuildVisualSurface();
	void UpdateVisualSurface();
	float GetWakeHeightAt(const FVector2D& XY, float TimeSeconds, bool bWavesEnabled) const;
	FVector GetWakeSurfaceVelocityAt(const FVector2D& XY, float TimeSeconds, bool bWavesEnabled) const;

	TArray<FGCWaterWakeImpulse> ActiveWakeImpulses;
	float VisualSurfaceUpdateAccumulator = 0.0f;
	TArray<FVector> VisualSurfaceVertices;
	TArray<FVector> VisualSurfaceNormals;
	TArray<FVector2D> VisualSurfaceUVs;
	TArray<FLinearColor> VisualSurfaceColors;
	TArray<FProcMeshTangent> VisualSurfaceTangents;
	TArray<int32> VisualSurfaceTriangles;
};
