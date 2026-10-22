#pragma once

#include "CoreMinimal.h"
#include "GCTypes.generated.h"

/** Which wheels receive engine torque. */
UENUM(BlueprintType)
enum class EGCVehicleDriveType : uint8
{
	RWD UMETA(DisplayName = "Rear Wheel Drive"),
	FWD UMETA(DisplayName = "Front Wheel Drive"),
	AWD UMETA(DisplayName = "All Wheel Drive")
};

UENUM(BlueprintType)
enum class EGCCameraMode : uint8
{
	Chase,
	Cockpit,
	Hood
};

/** Interactable world locations. Jobs/life-sim is intentionally milestone 2. */
UENUM(BlueprintType)
enum class EGCLocationType : uint8
{
	Hospital,
	Shop,
	PoliceStation,
	Airport,
	Marina,
	SkyscraperElevator
};

/** Placeholder roles - settable by dev command now, full job loops are milestone 3. */
UENUM(BlueprintType)
enum class EGCJob : uint8
{
	Civilian,
	Police,
	Medic,
	Taxi,
	ShopOwner,
	BorderPatrol,
	Hitman,
	President
};

UENUM(BlueprintType)
enum class EGCWeaponType : uint8
{
	Pistol,
	Rifle,
	SMG,
	Sniper
};
