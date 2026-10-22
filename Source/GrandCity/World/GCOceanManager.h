#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

/**
 * Single source of truth for the wave field.
 * Boats (buoyancy), swimming and later surfing all sample GetWaveHeightAt(),
 * so visuals and physics agree. Your water material should use the same 3 sine waves
 * (documented in Docs/WATER_MATERIAL.md) or the UE Water plugin's height query later.
 */
UCLASS()
class GRANDCITY_API AGCOceanManager : public AActor
{
	GENERATED_BODY()

public:
	AGCOceanManager();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ocean")
	UStaticMeshComponent* WaterMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	float AmplitudeScale = 1.0f;

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

	static FGCOceanSample SampleOceanStatic(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale);
};
