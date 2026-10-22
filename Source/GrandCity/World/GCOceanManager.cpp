#include "World/GCOceanManager.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GCGameUserSettings.h"

AGCOceanManager::AGCOceanManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	Tags.Add(TEXT("Water"));

	WaterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterMesh"));
	RootComponent = WaterMesh;
	WaterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WaveSurface = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaveSurface"));
	WaveSurface->SetupAttachment(RootComponent);
	WaveSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGCOceanManager::BeginPlay()
{
	Super::BeginPlay();
	if (!bGenerateVisualSurface || !WaveSurface)
	{
		return;
	}

	// Keep compatibility with maps that assign the material to the legacy root component.
	if (!WaveSurface->GetMaterial(0) && WaterMesh->GetMaterial(0))
	{
		WaveSurface->SetMaterial(0, WaterMesh->GetMaterial(0));
	}
	BuildVisualSurface();
	UpdateVisualSurface();
	SetActorTickEnabled(true);
}

void AGCOceanManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	ActiveWakeImpulses.RemoveAll([Now](const FGCWaterWakeImpulse& Wake)
	{
		return Now - Wake.StartTime > Wake.LifetimeSeconds;
	});

	if (!bGenerateVisualSurface || !WaveSurface)
	{
		if (ActiveWakeImpulses.IsEmpty())
		{
			SetActorTickEnabled(false);
		}
		return;
	}

	VisualSurfaceUpdateAccumulator += DeltaSeconds;
	const float UpdateInterval = FMath::Clamp(VisualSurfaceUpdateInterval, 0.016f, 0.25f);
	if (VisualSurfaceUpdateAccumulator >= UpdateInterval)
	{
		VisualSurfaceUpdateAccumulator = FMath::Fmod(VisualSurfaceUpdateAccumulator, UpdateInterval);
		UpdateVisualSurface();
	}
}

void AGCOceanManager::BuildVisualSurface()
{
	if (!WaveSurface)
	{
		return;
	}

	const int32 Resolution = FMath::Clamp(VisualSurfaceGridResolution, 8, 256);
	const int32 Side = Resolution + 1;
	const int32 VertexCount = Side * Side;
	const FVector2D Extent(FMath::Max(1000.0f, VisualSurfaceHalfExtent.X), FMath::Max(1000.0f, VisualSurfaceHalfExtent.Y));
	VisualSurfaceVertices.SetNumUninitialized(VertexCount);
	VisualSurfaceNormals.Init(FVector::UpVector, VertexCount);
	VisualSurfaceUVs.SetNumUninitialized(VertexCount);
	VisualSurfaceColors.Init(FLinearColor::White, VertexCount);
	VisualSurfaceTangents.Init(FProcMeshTangent(FVector::ForwardVector, false), VertexCount);
	VisualSurfaceTriangles.Reset(Resolution * Resolution * 6);

	for (int32 Y = 0; Y < Side; ++Y)
	{
		for (int32 X = 0; X < Side; ++X)
		{
			const int32 Index = Y * Side + X;
			VisualSurfaceVertices[Index] = FVector(-Extent.X + (2.0f * Extent.X * X / Resolution),
				-Extent.Y + (2.0f * Extent.Y * Y / Resolution), 0.0f);
			VisualSurfaceUVs[Index] = FVector2D(static_cast<float>(X) / Resolution, static_cast<float>(Y) / Resolution);
		}
	}

	for (int32 Y = 0; Y < Resolution; ++Y)
	{
		for (int32 X = 0; X < Resolution; ++X)
		{
			const int32 I = Y * Side + X;
			VisualSurfaceTriangles.Add(I);
			VisualSurfaceTriangles.Add(I + 1);
			VisualSurfaceTriangles.Add(I + Side);
			VisualSurfaceTriangles.Add(I + 1);
			VisualSurfaceTriangles.Add(I + Side + 1);
			VisualSurfaceTriangles.Add(I + Side);
		}
	}

	WaveSurface->CreateMeshSection_LinearColor(0, VisualSurfaceVertices, VisualSurfaceTriangles,
		VisualSurfaceNormals, VisualSurfaceUVs, VisualSurfaceColors, VisualSurfaceTangents, false);
}

void AGCOceanManager::UpdateVisualSurface()
{
	if (!WaveSurface || VisualSurfaceVertices.IsEmpty())
	{
		return;
	}

	const int32 Resolution = FMath::Clamp(VisualSurfaceGridResolution, 8, 256);
	const int32 Side = Resolution + 1;
	const FVector2D Extent(FMath::Max(1000.0f, VisualSurfaceHalfExtent.X), FMath::Max(1000.0f, VisualSurfaceHalfExtent.Y));
	const FVector ActorLocation = GetActorLocation();
	const float TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	for (int32 Y = 0; Y < Side; ++Y)
	{
		for (int32 X = 0; X < Side; ++X)
		{
			const int32 Index = Y * Side + X;
			const float LocalX = -Extent.X + (2.0f * Extent.X * X / Resolution);
			const float LocalY = -Extent.Y + (2.0f * Extent.Y * Y / Resolution);
			const FVector2D WorldXY(ActorLocation.X + LocalX, ActorLocation.Y + LocalY);
			const float Height = GetWaveHeightAt(WorldXY, TimeSeconds, AmplitudeScale)
				+ GetWakeHeightAt(WorldXY, TimeSeconds);
			VisualSurfaceVertices[Index] = FVector(LocalX, LocalY, Height);
		}
	}

	const float StepX = (2.0f * Extent.X) / Resolution;
	const float StepY = (2.0f * Extent.Y) / Resolution;
	for (int32 Y = 0; Y < Side; ++Y)
	{
		for (int32 X = 0; X < Side; ++X)
		{
			const int32 Left = Y * Side + FMath::Max(0, X - 1);
			const int32 Right = Y * Side + FMath::Min(Resolution, X + 1);
			const int32 Down = FMath::Max(0, Y - 1) * Side + X;
			const int32 Up = FMath::Min(Resolution, Y + 1) * Side + X;
			const float Ddx = StepX * (X == 0 || X == Resolution ? 1.0f : 2.0f);
			const float Ddy = StepY * (Y == 0 || Y == Resolution ? 1.0f : 2.0f);
			const float SlopeX = (VisualSurfaceVertices[Right].Z - VisualSurfaceVertices[Left].Z) / Ddx;
			const float SlopeY = (VisualSurfaceVertices[Up].Z - VisualSurfaceVertices[Down].Z) / Ddy;
			const FVector Normal = FVector(-SlopeX, -SlopeY, 1.0f).GetSafeNormal();
			VisualSurfaceNormals[Y * Side + X] = Normal;
			const float SafeAmplitude = FMath::Max(0.01f, FMath::Abs(AmplitudeScale));
			const float Crest = VisualSurfaceVertices[Y * Side + X].Z / (48.0f * SafeAmplitude) * 0.75f
				+ (1.0f - Normal.Z) * 2.2f;
			const float Foam = FMath::SmoothStep(0.55f, 1.0f, Crest);
			VisualSurfaceColors[Y * Side + X] = FLinearColor(Foam, 1.0f, 1.0f, 1.0f);
		}
	}

	WaveSurface->UpdateMeshSection_LinearColor(0, VisualSurfaceVertices, VisualSurfaceNormals,
		VisualSurfaceUVs, VisualSurfaceColors, VisualSurfaceTangents);
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
	// Three travelling waves (amplitudes in cm); the visual grid samples this exact function.
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

float AGCOceanManager::GetWakeHeightAt(const FVector2D& XY, float TimeSeconds) const
{
	if (const UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings())
	{
		if (!Settings->bWavesEnabled)
		{
			return 0.0f;
		}
	}

	float Height = 0.0f;
	for (const FGCWaterWakeImpulse& Wake : ActiveWakeImpulses)
	{
		const float Age = TimeSeconds - Wake.StartTime;
		if (Age < 0.0f || Age > Wake.LifetimeSeconds)
		{
			continue;
		}

		const FVector2D Offset = XY - Wake.Center;
		const float Back = -FVector2D::DotProduct(Offset, Wake.TravelDirection);
		const float MaxLength = FMath::Min(Age * Wake.GrowthSpeedCmPerSecond, 50000.0f);
		if (Back <= 0.0f || Back >= MaxLength)
		{
			continue;
		}

		const FVector2D Right(-Wake.TravelDirection.Y, Wake.TravelDirection.X);
		const float Side = FMath::Abs(FVector2D::DotProduct(Offset, Right));
		const float ArmDistance = Side - Back * 0.58f;
		const float ArmWidth = 90.0f + Back * 0.025f;
		const float Envelope = FMath::Exp(-FMath::Abs(ArmDistance) / ArmWidth)
			* FMath::Exp(-Back / 22000.0f) * FMath::Exp(-Age * 0.42f);
		const float Growth = 1.0f - FMath::Exp(-Age * 5.0f);
		const float Phase = Back * (2.0f * PI / Wake.WavelengthCm) - Age * 4.4f;
		Height += Wake.StrengthCm * Envelope * Growth * FMath::Sin(Phase);
	}
	return Height;
}

void AGCOceanManager::EmitWake(const FVector& WorldLocation, const FVector& TravelDirection, float StrengthCm,
	float LifetimeSeconds, float GrowthSpeedCmPerSecond, float WavelengthCm)
{
	if (!GetWorld() || !FMath::IsFinite(StrengthCm) || StrengthCm <= 0.05f)
	{
		return;
	}
	const FVector2D Direction(TravelDirection.X, TravelDirection.Y);
	if (Direction.IsNearlyZero())
	{
		return;
	}

	if (ActiveWakeImpulses.Num() >= 64)
	{
		ActiveWakeImpulses.RemoveAt(0);
	}
	FGCWaterWakeImpulse& Wake = ActiveWakeImpulses.AddDefaulted_GetRef();
	Wake.Center = FVector2D(WorldLocation.X, WorldLocation.Y);
	Wake.TravelDirection = Direction.GetSafeNormal();
	Wake.StartTime = GetWorld()->GetTimeSeconds();
	Wake.StrengthCm = FMath::Clamp(StrengthCm, 0.0f, 40.0f);
	Wake.LifetimeSeconds = FMath::Clamp(LifetimeSeconds, 0.25f, 8.0f);
	Wake.GrowthSpeedCmPerSecond = FMath::Clamp(GrowthSpeedCmPerSecond, 100.0f, 4000.0f);
	Wake.WavelengthCm = FMath::Clamp(WavelengthCm, 250.0f, 5000.0f);
	SetActorTickEnabled(true);
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
	const FVector2D XY(WorldLocation.X, WorldLocation.Y);
	FGCOceanSample Sample = SampleOceanStatic(XY, Time, AmplitudeScale);
	const float WakeHeight = GetWakeHeightAt(XY, Time);
	Sample.Height += GetActorLocation().Z + WakeHeight;

	// Add the wake's finite-difference slope and vertical motion to the same sample used by buoyancy.
	constexpr float SpatialStepCm = 50.0f;
	constexpr float TimeStepSeconds = 0.02f;
	const float WakeSlopeX = (GetWakeHeightAt(XY + FVector2D(SpatialStepCm, 0.0f), Time)
		- GetWakeHeightAt(XY - FVector2D(SpatialStepCm, 0.0f), Time)) / (2.0f * SpatialStepCm);
	const float WakeSlopeY = (GetWakeHeightAt(XY + FVector2D(0.0f, SpatialStepCm), Time)
		- GetWakeHeightAt(XY - FVector2D(0.0f, SpatialStepCm), Time)) / (2.0f * SpatialStepCm);
	const float WakeVerticalVelocity = (GetWakeHeightAt(XY, Time + TimeStepSeconds)
		- GetWakeHeightAt(XY, Time - TimeStepSeconds)) / (2.0f * TimeStepSeconds);
	const float BaseSlopeX = -Sample.Normal.X / FMath::Max(0.001f, Sample.Normal.Z);
	const float BaseSlopeY = -Sample.Normal.Y / FMath::Max(0.001f, Sample.Normal.Z);
	Sample.Normal = FVector(-(BaseSlopeX + WakeSlopeX), -(BaseSlopeY + WakeSlopeY), 1.0f).GetSafeNormal();
	Sample.VerticalVelocity += WakeVerticalVelocity;
	const float WakeFoam = FMath::Clamp(FMath::Abs(WakeHeight) / 16.0f
		+ (FMath::Abs(WakeSlopeX) + FMath::Abs(WakeSlopeY)) * 0.3f, 0.0f, 1.0f);
	Sample.FoamFactor = FMath::Max(Sample.FoamFactor, WakeFoam);
	return Sample;
}
