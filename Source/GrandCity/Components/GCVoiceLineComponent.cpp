#include "Components/GCVoiceLineComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundBase.h"

UGCVoiceLineComponent::UGCVoiceLineComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGCVoiceLineComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Owner = GetOwner())
	{
		Audio = NewObject<UAudioComponent>(Owner, TEXT("VoiceAudio"));
		Audio->RegisterComponent();
		if (USceneComponent* Root = Owner->GetRootComponent())
		{
			Audio->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
		}
		Audio->bAutoActivate = false;
		Audio->SetVolumeMultiplier(Volume);
	}
}

const FGCVoiceLine* UGCVoiceLineComponent::FindLine(FName LineID) const
{
	for (const FGCVoiceLine& Line : Lines)
	{
		if (Line.LineID == LineID)
		{
			return &Line;
		}
	}
	return nullptr;
}

bool UGCVoiceLineComponent::PlayLine(FName LineID, FString& OutSubtitle)
{
	OutSubtitle.Empty();
	const FGCVoiceLine* Line = FindLine(LineID);
	if (!Line)
	{
		return false;
	}
	return PlayFoundLine(*Line, OutSubtitle);
}

bool UGCVoiceLineComponent::PlayRandomWithPrefix(const FString& Prefix, FString& OutSubtitle)
{
	OutSubtitle.Empty();
	TArray<const FGCVoiceLine*> Matches;
	for (const FGCVoiceLine& Line : Lines)
	{
		if (Line.LineID.ToString().StartsWith(Prefix))
		{
			Matches.Add(&Line);
		}
	}
	if (Matches.Num() == 0)
	{
		return false;
	}
	return PlayFoundLine(*Matches[FMath::RandRange(0, Matches.Num() - 1)], OutSubtitle);
}

bool UGCVoiceLineComponent::PlayFoundLine(const FGCVoiceLine& Line, FString& OutSubtitle)
{
	if (!GetWorld())
	{
		return false;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (const float* Last = LastPlayedTimes.Find(Line.LineID))
	{
		if (Now - *Last < Line.CooldownSeconds)
		{
			return false;
		}
	}
	LastPlayedTimes.Add(Line.LineID, Now);

	float Duration = FMath::Clamp(Line.Subtitle.Len() * 0.055f, 2.0f, 12.0f);
	if (Line.Sound && Audio)
	{
		Audio->SetSound(Line.Sound);
		Audio->SetVolumeMultiplier(Volume);
        Audio->SetPitchMultiplier(FMath::Clamp(PitchMultiplier, 0.5f, 1.5f));
		Audio->Play();
		const float SoundDuration = Line.Sound->GetDuration();
		if (SoundDuration > 0.1f && SoundDuration < 120.0f)
		{
			Duration = SoundDuration;
		}
	}

	OutSubtitle = Line.Subtitle;
	OnSubtitle.Broadcast(OutSubtitle, Duration);
	return true;
}

void UGCVoiceLineComponent::StopLine()
{
	if (Audio)
	{
		Audio->Stop();
	}
}

bool UGCVoiceLineComponent::IsSpeaking() const
{
	return Audio && Audio->IsPlaying();
}
