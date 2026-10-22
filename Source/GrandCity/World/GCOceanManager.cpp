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
	const float A = FMath::Clamp(FMath::IsFinite(InAmplitudeScale) ? InAmplitudeScale : 0.0f, 0.0f, 4.0f);
	// Three travelling waves (amplitudes in cm). Keep the same coefficients in the water material.
	const float P0 = 0.0016f * XY.X + 0.0011f * XY.Y + TimeSeconds * 0.9f;
	const float P1 = -0.0023f * XY.X + 0.0028f * XY.Y + TimeSeconds * 1.35f;
	const float P2 = 0.0061f * XY.X - 0.0052f * XY.Y + TimeSeconds * 2.2f;
	return A * (26.0f * FMath::Sin(P0) + 15.0f * FMath::Sin(P1) + 7.0f * FMath::Sin(P2));
}

FVector AGCOceanManager::GetWaveNormalAt(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale)
{
	if (const UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings())
	{
		if (!Settings->bWavesEnabled)
		{
			return FVector::UpVector;
		}
	}
	const float A = FMath::Clamp(FMath::IsFinite(InAmplitudeScale) ? InAmplitudeScale : 0.0f, 0.0f, 4.0f);
	const float C0 = FMath::Cos(0.0016f * XY.X + 0.0011f * XY.Y + TimeSeconds * 0.9f);
	const float C1 = FMath::Cos(-0.0023f * XY.X + 0.0028f * XY.Y + TimeSeconds * 1.35f);
	const float C2 = FMath::Cos(0.0061f * XY.X - 0.0052f * XY.Y + TimeSeconds * 2.2f);
	const float Dx = A * (26.0f * 0.0016f * C0 - 15.0f * 0.0023f * C1 + 7.0f * 0.0061f * C2);
	const float Dy = A * (26.0f * 0.0011f * C0 + 15.0f * 0.0028f * C1 - 7.0f * 0.0052f * C2);
	return FVector(-Dx, -Dy, 1.0f).GetSafeNormal();
}

FVector AGCOceanManager::GetSurfaceVelocityAt(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale)
{
	FVector Velocity = GetCurrentAt(XY, TimeSeconds);
	if (const UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings())
	{
		if (!Settings->bWavesEnabled)
		{
			return Velocity;
		}
	}
	const float A = FMath::Clamp(FMath::IsFinite(InAmplitudeScale) ? InAmplitudeScale : 0.0f, 0.0f, 4.0f);
	const float P0 = 0.0016f * XY.X + 0.0011f * XY.Y + TimeSeconds * 0.9f;
	const float P1 = -0.0023f * XY.X + 0.0028f * XY.Y + TimeSeconds * 1.35f;
	const float P2 = 0.0061f * XY.X - 0.0052f * XY.Y + TimeSeconds * 2.2f;
	const float C0 = A * 26.0f * 0.9f * FMath::Cos(P0) / FMath::Sqrt(FMath::Square(0.0016f) + FMath::Square(0.0011f));
	const float C1 = A * 15.0f * 1.35f * FMath::Cos(P1) / FMath::Sqrt(FMath::Square(0.0023f) + FMath::Square(0.0028f));
	const float C2 = A * 7.0f * 2.2f * FMath::Cos(P2) / FMath::Sqrt(FMath::Square(0.0061f) + FMath::Square(0.0052f));
	Velocity += FVector(0.0016f * C0 - 0.0023f * C1 + 0.0061f * C2,
		0.0011f * C0 + 0.0028f * C1 - 0.0052f * C2, 0.0f);
	return Velocity;
}

float AGCOceanManager::GetWaveHeightWorld(const FVector& WorldLocation) const
{
	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return GetActorLocation().Z + GetWaveHeightAt(FVector2D(WorldLocation.X, WorldLocation.Y), Time, AmplitudeScale);
}

float AGCOceanManager::GetWaveVerticalVelocityAt(const FVector2D& XY, float TimeSeconds, float InAmplitudeScale)
{
	if (const UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings())
	{
		if (!Settings->bWavesEnabled)
		{
			return 0.0f;
		}
	}
	const float A = FMath::Clamp(FMath::IsFinite(InAmplitudeScale) ? InAmplitudeScale : 0.0f, 0.0f, 4.0f);
	return A * (26.0f * 0.9f * FMath::Cos(0.0016f * XY.X + 0.0011f * XY.Y + TimeSeconds * 0.9f)
		+ 15.0f * 1.35f * FMath::Cos(-0.0023f * XY.X + 0.0028f * XY.Y + TimeSeconds * 1.35f)
		+ 7.0f * 2.2f * FMath::Cos(0.0061f * XY.X - 0.0052f * XY.Y + TimeSeconds * 2.2f));
}

FVector AGCOceanManager::GetCurrentAt(const FVector2D& XY, float TimeSeconds)
{
	// Gentle low-frequency background flow (about a ten-minute rotational cycle).
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
	Sample.SurfaceVelocity = GetSurfaceVelocityAt(XY, TimeSeconds, InAmplitudeScale);
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
