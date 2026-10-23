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
	Treads->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Treads->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	TreadDetailVisuals = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TreadDetailVisuals"));
	TreadDetailVisuals->SetupAttachment(SceneRoot);
	TreadDetailVisuals->SetMobility(EComponentMobility::Movable);
	TreadDetailVisuals->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

void AGCEscalatorActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (int32 Index = Riders.Num() - 1; Index >= 0; --Index)
	{
		RestoreRider(Index);
	}
	Super::EndPlay(EndPlayReason);
}

void AGCEscalatorActor::RebuildPathAndTreads()
{
	if (!TreadPath || !Treads || !TreadDetailVisuals || !RideTrigger)
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
	TreadDetailVisuals->ClearInstances();
	Treads->SetStaticMesh(TreadMesh);
	TreadDetailVisuals->SetStaticMesh(TreadDetailMesh);
	TreadDetailVisuals->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Keep robust engine-cube collision while rendering the imported ribbed tread when available.
	Treads->SetVisibility(TreadDetailMesh == nullptr, true);
	TreadDetailVisuals->SetVisibility(TreadDetailMesh != nullptr, true);
	Treads->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Treads->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Treads->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
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
		if (TreadDetailMesh)
		{
			TreadDetailVisuals->AddInstance(TreadTransform, false);
		}
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
		if (TreadDetailMesh && TreadDetailVisuals && TreadDetailVisuals->GetInstanceCount() == Count)
		{
			TreadDetailVisuals->UpdateInstanceTransform(Index, TreadTransform, false, Index == Count - 1, true);
		}
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
		ACharacter* Rider = Riders[Index].Character.Get();
		if (!IsValid(Rider))
		{
			Riders.RemoveAt(Index);
			continue;
		}

		UCharacterMovementComponent* Movement = Rider->GetCharacterMovement();
		if (!Movement)
		{
			RestoreRider(Index);
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
			RestoreRider(Index);
			continue;
		}

		const FVector LocalRideDirection = FVector(1.0f, 0.0f, Rise / Run).GetSafeNormal();
		const FVector WorldRideDirection = GetActorTransform().TransformVectorNoScale(LocalRideDirection).GetSafeNormal();
		// Walking mode discards vertical input, so riders use collision-swept flying movement while
		// inside the escalator's ride corridor. Treads overlap Pawns to avoid blocking the capsule.
		Movement->MaxFlySpeed = SafeStepSpeed;
		Movement->AddInputVector(WorldRideDirection, true);
	}
}

void AGCEscalatorActor::OnRideBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		if (!Movement || Movement->IsFalling())
		{
			return;
		}

		const bool bAlreadyRiding = Riders.ContainsByPredicate([Character](const FRiderState& State)
		{
			return State.Character.Get() == Character;
		});
		if (!bAlreadyRiding)
		{
			FRiderState State;
			State.Character = Character;
			State.PreviousMovementMode = static_cast<uint8>(Movement->MovementMode);
			State.PreviousCustomMovementMode = Movement->CustomMovementMode;
			State.PreviousMaxFlySpeed = Movement->MaxFlySpeed;
			Riders.Add(State);
			Movement->SetMovementMode(MOVE_Flying);
		}
	}
}

void AGCEscalatorActor::OnRideEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		for (int32 Index = Riders.Num() - 1; Index >= 0; --Index)
		{
			if (!Riders[Index].Character.IsValid() || Riders[Index].Character.Get() == Character)
			{
				RestoreRider(Index);
			}
		}
	}
}

void AGCEscalatorActor::RestoreRider(int32 Index)
{
	if (!Riders.IsValidIndex(Index))
	{
		return;
	}

	const FRiderState State = Riders[Index];
	Riders.RemoveAt(Index);
	if (ACharacter* Character = State.Character.Get())
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->MaxFlySpeed = State.PreviousMaxFlySpeed;
			Movement->SetMovementMode(static_cast<EMovementMode>(State.PreviousMovementMode), State.PreviousCustomMovementMode);
		}
	}
}
