#include "Core/GCPlayerState.h"

AGCPlayerState::AGCPlayerState()
{
}

void AGCPlayerState::AddMoney(int32 Amount)
{
	Money = FMath::Max(0, Money + Amount);
}

bool AGCPlayerState::SpendMoney(int32 Amount)
{
	if (Money < Amount)
	{
		return false;
	}
	Money -= Amount;
	return true;
}

void AGCPlayerState::AddWantedStars(int32 Count)
{
	const int32 Old = WantedStars;
	WantedStars = FMath::Clamp(WantedStars + Count, 0, 5);
	HiddenTime = 0.0f;
	if (WantedStars != Old)
	{
		OnWantedChanged.Broadcast(WantedStars);
	}
}

void AGCPlayerState::ClearWanted()
{
	if (WantedStars != 0)
	{
		WantedStars = 0;
		OnWantedChanged.Broadcast(0);
	}
	HiddenTime = 0.0f;
}

void AGCPlayerState::TickWantedDecay(float DeltaSeconds, bool bSeenByPolice)
{
	if (WantedStars <= 0)
	{
		return;
	}
	if (bSeenByPolice)
	{
		HiddenTime = 0.0f;
		return;
	}
	HiddenTime += DeltaSeconds;
	if (HiddenTime >= WantedDecaySeconds)
	{
		HiddenTime = 0.0f;
		WantedStars = FMath::Max(0, WantedStars - 1);
		OnWantedChanged.Broadcast(WantedStars);
	}
}
