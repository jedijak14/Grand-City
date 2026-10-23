#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCDayNightManager.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class UInstancedStaticMeshComponent;

/**
 * Day/night cycle with a star dome that includes real bright-star positions
 * (RA/Dec) for Ursa Major, Lyra, Lyra, Cassiopeia and Scorpius, plus seeded fill stars.
 * Pair with a Sky Atmosphere actor for the sky gradient itself (see README map setup).
 */
UCLASS()
class GRANDCITY_API AGCDayNightManager : public AActor
{
	GENERATED_BODY()

public:
	AGCDayNightManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DayNight")
	UDirectionalLightComponent* SunLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DayNight")
	UDirectionalLightComponent* MoonLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DayNight")
	USkyLightComponent* FillSkyLight;

	/** Assign a small sphere mesh + emissive white material for stars. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DayNight")
	UInstancedStaticMeshComponent* Stars;

	/** 0-24. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	float TimeOfDay = 10.0f;

	/** If true, the cycle length comes from the settings screen value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	bool bUseSettingsDayLength = true;

	UPROPERTY(BlueprintReadOnly, Category = "DayNight")
	float NightFactor = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "DayNight|Calendar")
    int64 GameDayIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "DayNight")
	void SetTimeOfDay(float Hour);

    UFUNCTION(BlueprintCallable, Category = "DayNight|Calendar")
    void SetGameClock(float Hour, int64 DayIndex);

private:
	void GenerateStars();
	void ApplyTimeOfDay();
};
