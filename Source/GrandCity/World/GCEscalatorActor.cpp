#include "World/GCEscalatorActor.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AGCEscalatorActor::AGCEscalatorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f / 30.0f;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	TreadPath = CreateDefaultSubobject<USplineComponent>(TEXT("TreadPath"));
	TreadPath->SetupAttachment(SceneRoot);

	Treads = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MovingTreads"));
	Treads->SetupAttachment(SceneRoot);
	Treads->SetMobility(EComponentMobility::Movable);
	Treads->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Treads->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	RideTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("RideTrigger"));
	RideTrigger->SetupAttachment(SceneRoot);
	RideTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RideTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	RideTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	RideTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RideTrigger->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultTread(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultTread.Succeeded())
	{
		TreadMesh = DefaultTread.Object;
	}
}

void AGCEscalatorActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildPathAndTreads();
}

void AGCEscalatorActor::BeginPlay()
{
	Super::BeginPlay();
	RideTrigger->OnComponentBeginOverlap.AddDynamic(this, &AGCEscalatorActor::OnRideBegin);
	RideTrigger->OnComponentEndOverlap.AddDynamic(this, &AGCEscalatorActor::OnRideEnd);
	UpdateTreadTransforms();
}

void AGCEscalatorActor::RebuildPathAndTreads()
{
	if (!TreadPath || !Treads || !RideTrigger)
	{
		return;
	}

	const float Run = FMath::Max(200.0f, RampRunCm);
	const float Rise = FMath::Max(100.0f, RampRiseCm);
	TreadPath->ClearSplinePoints(false);
	const FVector Points[] = {
		FVector(0.0f, 0.0f, 0.0f),
		FVector(Run * 0.24f, 0.0f, Rise * 0.24f),
		FVector(Run * 0.50f, 0.0f, Rise * 0.50f),
		FVector(Run * 0.76f, 0.0f, Rise * 0.76f),
		FVector(Run, 0.0f, Rise),
		FVector(Run + 120.0f, 0.0f, Rise + 75.0f),
		FVector(Run + 150.0f, 0.0f, Rise - 140.0f),
		FVector(Run * 0.72f, 0.0f, -190.0f),
		FVector(Run * 0.25f, 0.0f, -190.0f),
		FVector(-90.0f, 0.0f, -100.0f)
	};
	for (const FVector& Point : Points)
	{
		TreadPath->AddSplinePoint(Point, ESplineCoordinateSpace::Local, false);
	}
	TreadPath->SetClosedLoop(true, false);
	for (int32 Index = 0; Index < TreadPath->GetNumberOfSplinePoints(); ++Index)
	{
		TreadPath->SetSplinePointType(Index, ESplinePointType::CurveAuto, false);
	}
	TreadPath->UpdateSpline();

	RideTrigger->SetBoxExtent(FVector(Run * 0.5f + 90.0f, WidthCm * 0.5f + 45.0f, Rise * 0.5f + 210.0f));
	RideTrigger->SetRelativeLocation(FVector(Run * 0.5f, 0.0f, Rise * 0.5f + 100.0f));

	Treads->ClearInstances();
	Treads->SetStaticMesh(TreadMesh);
	Treads->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Treads->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	const int32 Count = FMath::Clamp(StepCount, 8, 64);
	const float PathLength = TreadPath->GetSplineLength();
	if (PathLength <= KINDA_SMALL_NUMBER || !TreadMesh)
	{
		return;
	}

	const FVector Scale(FMath::Max(10.0f, TreadDimensionsCm.X) / 100.0f,
		FMath::Max(10.0f, WidthCm - 16.0f) / 100.0f,
		FMath::Max(5.0f, TreadDimensionsCm.Z) / 100.0f);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Distance = PathLength * static_cast<float>(Index) / Count;
		FTransform TreadTransform = TreadPath->GetTransformAtDistanceAlongSpline(
			Distance, ESplineCoordinateSpace::Local, true);
		TreadTransform.SetRotation(FQuat::Identity); // step surfaces stay level while their centres follow the loop
		TreadTransform.SetScale3D(Scale);
		Treads->AddInstance(TreadTransform, false);
	}
	UpdateTreadTransforms();
}

void AGCEscalatorActor::UpdateTreadTransforms()
{
	if (!TreadPath || !Treads || Treads->GetInstanceCount() == 0)
	{
		return;
	}

	const float PathLength = TreadPath->GetSplineLength();
	const int32 Count = Treads->GetInstanceCount();
	if (PathLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	TreadPhaseCm = FMath::Fmod(TreadPhaseCm, PathLength);
	const FVector Scale(FMath::Max(10.0f, TreadDimensionsCm.X) / 100.0f,
		FMath::Max(10.0f, WidthCm - 16.0f) / 100.0f,
		FMath::Max(5.0f, TreadDimensionsCm.Z) / 100.0f);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Distance = FMath::Fmod(TreadPhaseCm + PathLength * static_cast<float>(Index) / Count, PathLength);
		FTransform TreadTransform = TreadPath->GetTransformAtDistanceAlongSpline(
			Distance, ESplineCoordinateSpace::Local, true);
		TreadTransform.SetRotation(FQuat::Identity);
		TreadTransform.SetScale3D(Scale);
		Treads->UpdateInstanceTransform(Index, TreadTransform, false, Index == Count - 1, true);
	}
}

void AGCEscalatorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!TreadPath || !Treads)
	{
		return;
	}

	const float PathLength = TreadPath->GetSplineLength();
	if (PathLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const float SafeStepSpeed = FMath::Clamp(FMath::IsFinite(StepSpeedCmPerSecond) ? StepSpeedCmPerSecond : 42.0f, 8.0f, 80.0f);
	TreadPhaseCm = FMath::Fmod(TreadPhaseCm + SafeStepSpeed * DeltaSeconds, PathLength);
	UpdateTreadTransforms();

	const float Run = FMath::Max(200.0f, RampRunCm);
	const float Rise = FMath::Max(100.0f, RampRiseCm);
	for (int32 Index = Riders.Num() - 1; Index >= 0; --Index)
	{
		ACharacter* Rider = Riders[Index].Get();
		if (!IsValid(Rider))
		{
			Riders.RemoveAt(Index);
			continue;
		}

		const UCharacterMovementComponent* Movement = Rider->GetCharacterMovement();
		if (!Movement || Movement->IsFalling())
		{
			continue;
		}

		const FVector LocalPosition = GetActorTransform().InverseTransformPosition(Rider->GetActorLocation());
		const float SurfaceZ = (LocalPosition.X / Run) * Rise;
		const float CharacterHalfHeight = Rider->GetCapsuleComponent()
			? Rider->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.0f;
		const float HeightAboveTread = LocalPosition.Z - SurfaceZ;
		if (LocalPosition.X < -60.0f || LocalPosition.X > Run + 80.0f
			|| FMath::Abs(LocalPosition.Y) > WidthCm * 0.5f
			|| HeightAboveTread < CharacterHalfHeight * 0.35f
			|| HeightAboveTread > CharacterHalfHeight * 2.2f)
		{
			continue;
		}

		const FVector LocalRideDirection = FVector(1.0f, 0.0f, Rise / Run).GetSafeNormal();
		const FVector WorldRideDirection = GetActorTransform().TransformVectorNoScale(LocalRideDirection).GetSafeNormal();
		const float InputScale = FMath::Clamp(SafeStepSpeed / FMath::Max(1.0f, Movement->MaxWalkSpeed), 0.05f, 0.25f);
		// Feed movement through CharacterMovement so it can step over tread risers and respect collision.
		Movement->AddInputVector(WorldRideDirection * InputScale, true);
	}
}

void AGCEscalatorActor::OnRideBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		const TWeakObjectPtr<ACharacter> WeakCharacter(Character);
		if (!Riders.Contains(WeakCharacter))
		{
			Riders.Add(WeakCharacter);
		}
	}
}

void AGCEscalatorActor::OnRideEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		Riders.RemoveAll([Character](const TWeakObjectPtr<ACharacter>& Rider)
		{
			return !Rider.IsValid() || Rider.Get() == Character;
		});
	}
}
