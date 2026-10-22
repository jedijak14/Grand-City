#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GCVoiceLineComponent.generated.h"

class USoundBase;
class UAudioComponent;

USTRUCT(BlueprintType)
struct FGCVoiceLine
{
	GENERATED_BODY()

	/** e.g. "CheckInGreeting", "SafetyBriefing", "AmbientGreeting". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LineID;

	/** Generated .wav imported from AudioVoice/ (see Docs/VOICE_LINES.md). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundBase* Sound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Subtitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CooldownSeconds = 9.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGCOnVoiceSubtitle, FString, Subtitle, float, Duration);

/**
 * Drop this on any actor (airport computer, airliner, pedestrian, police)
 * and fill Lines in a Blueprint child. Plays audio + broadcasts subtitles.
 * If a Sound is not assigned yet, subtitles still work so gameplay is testable.
 */
UCLASS(ClassGroup = (GrandCity), meta = (BlueprintSpawnableComponent))
class GRANDCITY_API UGCVoiceLineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGCVoiceLineComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice")
	TArray<FGCVoiceLine> Lines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float Volume = 1.0f;

	UPROPERTY(BlueprintAssignable, Category = "Voice")
	FGCOnVoiceSubtitle OnSubtitle;

	/** Returns true if the line exists (audio optional). OutSubtitle always filled when known. */
	UFUNCTION(BlueprintCallable, Category = "Voice")
	bool PlayLine(FName LineID, FString& OutSubtitle);

	/** Plays a random line whose ID starts with Prefix (e.g. "Ambient"). */
	UFUNCTION(BlueprintCallable, Category = "Voice")
	bool PlayRandomWithPrefix(const FString& Prefix, FString& OutSubtitle);

	UFUNCTION(BlueprintCallable, Category = "Voice")
	void StopLine();

	UFUNCTION(BlueprintCallable, Category = "Voice")
	bool IsSpeaking() const;

private:
	UPROPERTY(Transient)
	UAudioComponent* Audio = nullptr;

	TMap<FName, float> LastPlayedTimes;
	const FGCVoiceLine* FindLine(FName LineID) const;
	bool PlayFoundLine(const FGCVoiceLine& Line, FString& OutSubtitle);
};
