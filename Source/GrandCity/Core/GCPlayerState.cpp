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
	if (Amount < 0)
	{
		return false;
	}
	if (SelectedPaymentMethod == EGCPaymentMethod::Cash)
	{
		if (Money < Amount)
		{
			return false;
		}
		Money -= Amount;
		return true;
	}
	if (SelectedPaymentMethod == EGCPaymentMethod::BaylineCredit)
	{
		if (CreditUsed + Amount > CreditLimit)
		{
			return false;
		}
		CreditUsed += Amount;
		return true;
	}
	if (DigitalBalance < Amount)
	{
		return false;
	}
	DigitalBalance -= Amount;
	return true;
}

void AGCPlayerState::RefundMoney(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	if (SelectedPaymentMethod == EGCPaymentMethod::Cash)
	{
		AddMoney(Amount);
	}
	else if (SelectedPaymentMethod == EGCPaymentMethod::BaylineCredit)
	{
		CreditUsed = FMath::Max(0, CreditUsed - Amount);
	}
	else
	{
		DigitalBalance = FMath::Max(0, DigitalBalance + Amount);
	}
}

bool AGCPlayerState::RepayCreditCard(int32 Amount)
{
	const int32 Payment = FMath::Min(FMath::Max(0, Amount), CreditUsed);
	if (Payment == 0 || DigitalBalance < Payment)
	{
		return false;
	}
	DigitalBalance -= Payment;
	CreditUsed -= Payment;
	return true;
}

void AGCPlayerState::CyclePaymentMethod()
{
	const uint8 MethodCount = 5;
	SelectedPaymentMethod = static_cast<EGCPaymentMethod>((static_cast<uint8>(SelectedPaymentMethod) + 1) % MethodCount);
}

FString AGCPlayerState::GetPaymentMethodLabel() const
{
	switch (SelectedPaymentMethod)
	{
	case EGCPaymentMethod::BaylineTap: return TEXT("Bayline Tap");
	case EGCPaymentMethod::CoastPass: return TEXT("CoastPass");
	case EGCPaymentMethod::HarborCard: return TEXT("Harbor Card");
	case EGCPaymentMethod::BaylineCredit: return TEXT("Bayline Credit");
	case EGCPaymentMethod::Cash:
	default: return TEXT("Cash");
	}
}

bool AGCPlayerState::CreateCitizenIdentity(const FString& DisplayName)
{
	FString CleanName = DisplayName;
	CleanName.TrimStartAndEndInline();
	if (CleanName.Len() < 2)
	{
		return false;
	}
	CitizenName = CleanName.Left(24);
	CitizenID = FString::Printf(TEXT("GL-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8).ToUpper());
	return true;
}

int32 AGCPlayerState::TickJobIncome(float DeltaSeconds)
{
	if (CurrentJob != LastPaidJob)
	{
		LastPaidJob = CurrentJob;
		JobPayTimer = 0.0f;
	}
	if (CurrentJob == EGCJob::Civilian)
	{
		return 0;
	}
	JobPayTimer += FMath::Max(0.0f, DeltaSeconds);
	if (JobPayTimer < 120.0f)
	{
		return 0;
	}
	JobPayTimer = FMath::Fmod(JobPayTimer, 120.0f);
	int32 Wage = 200;
	switch (CurrentJob)
	{
	case EGCJob::Police: Wage = 350; break;
	case EGCJob::Medic: Wage = 300; break;
	case EGCJob::Taxi: Wage = 220; break;
	case EGCJob::ShopOwner: Wage = 260; break;
	case EGCJob::BorderPatrol: Wage = 400; break;
	case EGCJob::Hitman: Wage = 280; break;
	case EGCJob::President: Wage = 600; break;
	default: return 0;
	}
	DigitalBalance += Wage;
	return Wage;
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

bool AGCPlayerState::OwnsProperty(const FString& PropertyID) const
{
	return !PropertyID.IsEmpty() && OwnedPropertyIDs.ContainsByPredicate([&PropertyID](const FString& OwnedID)
	{
		return OwnedID.Equals(PropertyID, ESearchCase::IgnoreCase);
	});
}

bool AGCPlayerState::AddOwnedProperty(const FString& PropertyID, bool bStolen)
{
	if (PropertyID.IsEmpty() || OwnsProperty(PropertyID))
	{
		return false;
	}
	OwnedPropertyIDs.Add(PropertyID);
	if (bStolen)
	{
		StolenPropertyIDs.Add(PropertyID);
	}
	return true;
}

bool AGCPlayerState::OwnsDevice(const FString& DeviceID) const
{
	return !DeviceID.IsEmpty() && OwnedDeviceIDs.ContainsByPredicate([&DeviceID](const FString& OwnedID)
	{
		return OwnedID.Equals(DeviceID, ESearchCase::IgnoreCase);
	});
}

void AGCPlayerState::AddOwnedDevice(const FString& DeviceID, bool bPhone, const FString& PhoneModel, const FString& PhoneOS)
{
	if (DeviceID.IsEmpty())
	{
		return;
	}
	if (!OwnsDevice(DeviceID))
	{
		OwnedDeviceIDs.Add(DeviceID);
	}
	if (bPhone)
	{
		ActivePhoneModel = PhoneModel;
		ActivePhoneOS = PhoneOS;
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
