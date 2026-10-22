#include "World/GCOceanManager.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GCGameUserSettings.h"

AGCOceanManager::AGCOceanManager()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.Add(TEXT("Water"));

	WaterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterMesh"));
	RootComponent = WaterMesh;
	WaterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

float AGCOceanManager::GetWaveHeightAt(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale)
{
	if (const UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings())
	{
		if (!Settings->bWavesEnabled)
		{
			return 0.0f;
		}
	}
	const float A = InAmplitudeScale;
	// Three travelling sine waves (amplitudes in cm). Mirror these in the water material.
	float H = 0.0f;
	H += 26.0f * FMath::Sin(0.0016f * XY.X + 0.0011f * XY.Y + TimeSeconds * 0.9f);
	H += 15.0f * FMath::Sin(-0.0023f * XY.X + 0.0028f * XY.Y + TimeSeconds * 1.35f);
	H += 7.0f * FMath::Sin(0.0061f * XY.X - 0.0052f * XY.Y + TimeSeconds * 2.2f);
	return H * A;
}

FVector AGCOceanManager::GetWaveNormalAt(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale)
{
	const float E = 120.0f;
	const float H  = GetWaveHeightAt(XY, TimeSeconds, InAmplitudeScale);
	const float HX = GetWaveHeightAt(XY + FVector2D(E, 0.0f), TimeSeconds, InAmplitudeScale);
	const float HY = GetWaveHeightAt(XY + FVector2D(0.0f, E), TimeSeconds, InAmplitudeScale);
	return FVector(-(HX - H) / E, -(HY - H) / E, 1.0f).GetSafeNormal();
}

float AGCOceanManager::GetWaveHeightWorld(const FVector& WorldLocation) const
{
	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return GetActorLocation().Z + GetWaveHeightAt(FVector2D(WorldLocation.X, WorldLocation.Y), Time, AmplitudeScale);
}

float AGCOceanManager::GetWaveVerticalVelocityAt(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale)
{
	const float E = 0.06f;
	const float H1 = GetWaveHeightAt(XY, TimeSeconds + E, InAmplitudeScale);
	const float H0 = GetWaveHeightAt(XY, TimeSeconds - E, InAmplitudeScale);
	return (H1 - H0) / (2.0f * E);
}

FVector AGCOceanManager::GetCurrentAt(const FVector2D& XY, float TimeSeconds)
{
	// Gentle rotating coastal current: full loop every ~10 minutes, stronger far from origin.
	const float Angle = TimeSeconds * 0.0105f + (XY.X + XY.Y) * 0.00001f;
	const float Strength = 26.0f; // cm/s
	return FVector(FMath::Cos(Angle) * Strength, FMath::Sin(Angle) * Strength, 0.0f);
}

float AGCOceanManager::GetFoamFactorAt(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale)
{
	if (InAmplitudeScale <= 0.01f)
	{
		return 0.0f;
	}
	const float H = GetWaveHeightAt(XY, TimeSeconds, InAmplitudeScale);
	const float NormalizedHeight = H / (48.0f * InAmplitudeScale); // ~-1..1
	const FVector N = GetWaveNormalAt(XY, TimeSeconds, InAmplitudeScale);
	const float Slope = 1.0f - N.Z; // 0 flat, grows on steep faces
	const float Crest = NormalizedHeight * 0.75f + Slope * 2.2f;
	return FMath::SmoothStep(0.55f, 1.0f, Crest);
}

FGCOceanSample AGCOceanManager::SampleOceanStatic(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale)
{
	FGCOceanSample Sample;
	Sample.Height = GetWaveHeightAt(XY, TimeSeconds, InAmplitudeScale);
	Sample.Normal = GetWaveNormalAt(XY, TimeSeconds, InAmplitudeScale);
	Sample.VerticalVelocity = GetWaveVerticalVelocityAt(XY, TimeSeconds, InAmplitudeScale);
	Sample.Current = GetCurrentAt(XY, TimeSeconds);
	Sample.FoamFactor = GetFoamFactorAt(XY, TimeSeconds, InAmplitudeScale);
	return Sample;
}

FGCOceanSample AGCOceanManager::SampleOcean(const FVector& WorldLocation) const
{
	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	FGCOceanSample Sample = SampleOceanStatic(FVector2D(WorldLocation.X, WorldLocation.Y), Time, AmplitudeScale);
	Sample.Height += GetActorLocation().Z;
	return Sample;
}
