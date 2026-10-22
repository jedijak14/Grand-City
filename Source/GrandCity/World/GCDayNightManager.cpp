#include "World/GCDayNightManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Core/GCGameUserSettings.h"
#include "Kismet/GameplayStatics.h"

AGCDayNightManager::AGCDayNightManager()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	SunLight->SetupAttachment(Root);
	SunLight->SetMobility(EComponentMobility::Movable);
	// UE5 directional-light units are lux; use a physically plausible sun scale for Lumen/exposure.
	SunLight->SetIntensity(80000.0f);
	SunLight->bAtmosphereSunLight = true;
	SunLight->bEnableLightShaftOcclusion = true;

	MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonLight"));
	MoonLight->SetupAttachment(Root);
	MoonLight->SetMobility(EComponentMobility::Movable);
	MoonLight->SetIntensity(0.0f);
	MoonLight->SetLightColor(FLinearColor(0.55f, 0.65f, 0.9f));

	FillSkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("FillSkyLight"));
	FillSkyLight->SetupAttachment(Root);
	FillSkyLight->SetMobility(EComponentMobility::Movable);

	Stars = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Stars"));
	Stars->SetupAttachment(Root);
	Stars->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stars->CastShadow = false;
}

void AGCDayNightManager::BeginPlay()
{
	Super::BeginPlay();
	GenerateStars();
	ApplyTimeOfDay();
}

void AGCDayNightManager::SetTimeOfDay(float Hour)
{
	TimeOfDay = FMath::Fmod(FMath::Max(0.0f, Hour), 24.0f);
	ApplyTimeOfDay();
}

void AGCDayNightManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	float DayLength = 600.0f;
	if (bUseSettingsDayLength)
	{
		if (const UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings())
		{
			DayLength = Settings->DayLengthSeconds;
		}
	}
	if (DayLength > 1.0f)
	{
		TimeOfDay = FMath::Fmod(TimeOfDay + DeltaSeconds * (24.0f / DayLength), 24.0f);
	}

	// Keep the sky rig centred over the player so the star dome never runs out.
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		const FVector P = PlayerPawn->GetActorLocation();
		SetActorLocation(FVector(P.X, P.Y, 0.0f));
	}

	ApplyTimeOfDay();
}

void AGCDayNightManager::ApplyTimeOfDay()
{
	// Sun elevation: peaks at 13:00, below horizon roughly 19:00-05:00.
	const float DayProgress = (TimeOfDay - 6.0f) / 12.0f;              // 0 at 6am, 1 at 6pm
	const float ElevationSin = FMath::Sin(FMath::Clamp(DayProgress, -0.3f, 1.3f) * PI);
	const float Azimuth = FMath::Lerp(-90.0f, 270.0f, DayProgress);    // east -> west
	const float ElevationDeg = ElevationSin * 62.0f;

	SunLight->SetWorldRotation(FRotator(-ElevationDeg, Azimuth, 0.0f));
	const float DayIntensity = FMath::Clamp(ElevationSin, 0.0f, 1.0f);
	SunLight->SetIntensity(DayIntensity * 80000.0f);
	SunLight->SetLightColor(FLinearColor::LerpUsingHSV(
		FLinearColor(1.0f, 0.45f, 0.2f), FLinearColor(1.0f, 0.96f, 0.88f), DayIntensity));

	NightFactor = FMath::Clamp(1.0f - DayIntensity * 1.6f, 0.0f, 1.0f);
	MoonLight->SetIntensity(NightFactor * 0.35f);
	MoonLight->SetWorldRotation(FRotator(-(30.0f + NightFactor * 25.0f), Azimuth + 180.0f, 0.0f));

	FillSkyLight->SetIntensity(0.15f + DayIntensity * 0.9f);
	Stars->SetVisibility(NightFactor > 0.25f);
}

namespace
{
	// name-less bright stars: RA (hours), Dec (degrees), magnitude-ish size
	struct FRealStar { float RA; float Dec; float Size; };
	// Ursa Major (Big Dipper), Lyra belt+shoulders, Lyra, Cassiopeia (W), Scorpius head, Ursa Major
	const FRealStar RealStars[] = {
		{11.06f, 61.75f, 1.6f}, {11.03f, 56.38f, 1.5f}, {11.90f, 53.69f, 1.4f}, {12.26f, 57.03f, 1.4f},
		{12.90f, 55.96f, 1.4f}, {13.40f, 54.93f, 1.5f}, {13.79f, 49.31f, 1.6f},
		{5.92f, 7.41f, 1.7f}, {5.42f, 6.35f, 1.6f}, {5.60f, -1.20f, 1.4f}, {5.53f, -1.94f, 1.4f},
		{5.47f, -2.45f, 1.4f}, {5.24f, -8.20f, 1.7f}, {5.78f, -9.67f, 1.5f},
		{18.62f, 38.78f, 1.9f}, {19.77f, 27.96f, 1.5f}, {20.70f, 40.26f, 1.6f}, {19.51f, 45.28f, 1.8f},
		{0.15f, 59.15f, 1.5f}, {0.68f, 56.54f, 1.4f}, {0.95f, 60.72f, 1.5f}, {1.43f, 60.24f, 1.4f}, {1.91f, 63.67f, 1.4f},
		{16.49f, -26.43f, 1.8f}, {15.58f, -26.11f, 1.4f}, {17.56f, -37.30f, 1.5f},
		{1.76f, 89.26f, 1.7f}, {19.85f, 8.87f, 1.8f}, {4.60f, 16.51f, 1.8f}, {6.75f, -16.72f, 2.1f},
		{7.66f, 5.22f, 1.7f}, {10.14f, 19.84f, 1.6f}, {13.42f, -11.16f, 1.8f}, {14.26f, 19.18f, 1.8f}
	};
}

void AGCDayNightManager::GenerateStars()
{
	if (!Stars || !Stars->GetStaticMesh())
	{
		return; // No star mesh assigned yet - manager still runs sun/moon/sky.
	}

	Stars->ClearInstances();
	const float Radius = 85000.0f;

	auto AddStar = [&](const FVector& Dir, float Size)
	{
		const FTransform T(FRotator::ZeroRotator, Dir * Radius, FVector(Size));
		Stars->AddInstance(T, false); // local space, so the dome follows this actor over the player
	};

	for (const FRealStar& S : RealStars)
	{
		const float RaRad = FMath::DegreesToRadians(S.RA / 24.0f * 360.0f);
		const float DecRad = FMath::DegreesToRadians(S.Dec);
		const FVector Dir(
			FMath::Cos(DecRad) * FMath::Cos(RaRad),
			FMath::Cos(DecRad) * FMath::Sin(RaRad),
			FMath::Sin(DecRad));
		AddStar(Dir, S.Size * 130.0f);
	}

	// Seeded fill so every run has the same sky.
	FRandomStream Rand(1337);
	for (int32 i = 0; i < 1400; ++i)
	{
		FVector Dir = Rand.VRand();
		Dir.Z = FMath::Abs(Dir.Z) * 0.92f + 0.04f; // upper hemisphere bias
		Dir.Normalize();
		AddStar(Dir, Rand.FRandRange(35.0f, 95.0f));
	}
}
