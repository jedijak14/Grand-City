#pragma once

#include "CoreMinimal.h"
#include "Vehicle/GCPlanePawn.h"
#include "GCHelicopterPawn.generated.h"

/** Rotorcraft variant using collective lift and the shared aircraft entry/exit controls. */
UCLASS(Blueprintable)
class GRANDCITY_API AGCHelicopterPawn : public AGCPlanePawn
{
	GENERATED_BODY()

public:
	AGCHelicopterPawn();
};
