#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "GCGameUserSettings.generated.h"

/**
 * All settings-screen toggles live here and are actually read by gameplay code
 * (blood FX, waves, cloth/hair, traffic density, physics substeps budget).
 */
UCLASS(config = GameUserSettings)
class GRANDCITY_API UGCGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UGCGameUserSettings();

	static UGCGameUserSettings* GetGCSettings();

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Grand City|FX")
	bool bBloodEnabled = true;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Grand City|World")
	bool bWavesEnabled = true;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Grand City|World")
	bool bGrassEnabled = true;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Grand City|FX")
	bool bClothAndHairPhysics = true;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Grand City|World", meta = (ClampMin = "0", ClampMax = "60"))
	int32 TrafficDensity = 14;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Grand City|World", meta = (ClampMin = "0", ClampMax = "80"))
	int32 PedestrianDensity = 24;

	/** Seconds for one full 24h cycle. 0 = frozen at StartHour. */
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Grand City|World")
	float DayLengthSeconds = 600.0f;

	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;
};
