#include "UI/GCHUD.h"

#include "Character/GCCharacter.h"
#include "Components/GCHealthComponent.h"
#include "Core/GCPlayerState.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Vehicle/GCVehiclePawn.h"

void AGCHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas || !PlayerOwner)
	{
		return;
	}

	const float Left = 32.0f;
	float Top = 28.0f;
	const float LineHeight = 25.0f;
	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!Font)
	{
		return;
	}

	if (const AGCPlayerState* State = PlayerOwner->GetPlayerState<AGCPlayerState>())
	{
		if (!State->CitizenID.IsEmpty())
		{
			DrawText(FString::Printf(TEXT("%s  /  %s"), *State->CitizenName, *State->CitizenID),
				FColor(210, 225, 230), Left, Top, Font, 0.9f);
			Top += LineHeight;
		}
		DrawText(FString::Printf(TEXT("CASH  $%d"), State->Money), FColor::White, Left, Top, Font, 1.0f);
		Top += LineHeight;
		DrawText(FString::Printf(TEXT("DIGITAL  $%d  /  %s"), State->DigitalBalance, *State->GetPaymentMethodLabel()),
			FColor(125, 230, 220), Left, Top, Font, 0.95f);
		Top += LineHeight;
		DrawText(FString::Printf(TEXT("CREDIT  $%d / $%d"), State->CreditUsed, State->CreditLimit),
			State->CreditUsed > 0 ? FColor(255, 206, 105) : FColor(180, 190, 195), Left, Top, Font, 0.9f);
		Top += LineHeight;
		DrawText(FString::Printf(TEXT("WANTED  %d / 5"), State->WantedStars),
			State->WantedStars > 0 ? FColor::Red : FColor::White, Left, Top, Font, 1.0f);
		Top += LineHeight;
	}

	const APawn* Pawn = PlayerOwner->GetPawn();
	const UGCHealthComponent* Health = nullptr;
	if (const AGCCharacter* Character = Cast<AGCCharacter>(Pawn))
	{
		Health = Character->Health;
	}
	else if (const AGCVehiclePawn* Vehicle = Cast<AGCVehiclePawn>(Pawn))
	{
		Health = Vehicle->Health;
		DrawText(FString::Printf(TEXT("%3.0f km/h   GEAR %d"), Vehicle->GetSpeedKmh(), Vehicle->CurrentGear),
			FColor::White, Left, Top, Font, 1.0f);
		Top += LineHeight;
	}

	if (Health)
	{
		DrawText(FString::Printf(TEXT("HEALTH  %3.0f / %3.0f"), Health->Health, Health->MaxHealth),
			Health->Health <= 25.0f ? FColor::Red : FColor::White, Left, Top, Font, 1.0f);
	}

	if (const AGCCharacter* Character = Cast<AGCCharacter>(Pawn); Character && Character->bBodycamActive)
	{
		const int32 TotalSeconds = FMath::FloorToInt(Character->BodycamElapsedSeconds);
		const int32 Hours = TotalSeconds / 3600;
		const int32 Minutes = (TotalSeconds / 60) % 60;
		const int32 Seconds = TotalSeconds % 60;
		const float Right = Canvas->ClipX - 275.0f;
		const FColor RecordRed(255, 55, 48);
		DrawText(FString::Printf(TEXT("REC  %02d:%02d:%02d"), Hours, Minutes, Seconds),
			RecordRed, Right, 30.0f, Font, 1.25f);
		DrawText(TEXT("POLICE BODYCAM  /  LIVE"), FColor::White, Right, 56.0f, Font, 0.9f);

		const float MarginX = 24.0f;
		const float MarginY = 24.0f;
		const float Corner = 44.0f;
		DrawLine(MarginX, MarginY, MarginX + Corner, MarginY, RecordRed, 1.5f);
		DrawLine(MarginX, MarginY, MarginX, MarginY + Corner, RecordRed, 1.5f);
		DrawLine(Canvas->ClipX - MarginX, MarginY, Canvas->ClipX - MarginX - Corner, MarginY, RecordRed, 1.5f);
		DrawLine(Canvas->ClipX - MarginX, MarginY, Canvas->ClipX - MarginX, MarginY + Corner, RecordRed, 1.5f);
		DrawLine(MarginX, Canvas->ClipY - MarginY, MarginX + Corner, Canvas->ClipY - MarginY, RecordRed, 1.5f);
		DrawLine(MarginX, Canvas->ClipY - MarginY, MarginX, Canvas->ClipY - MarginY - Corner, RecordRed, 1.5f);
		DrawLine(Canvas->ClipX - MarginX, Canvas->ClipY - MarginY, Canvas->ClipX - MarginX - Corner, Canvas->ClipY - MarginY, RecordRed, 1.5f);
		DrawLine(Canvas->ClipX - MarginX, Canvas->ClipY - MarginY, Canvas->ClipX - MarginX, Canvas->ClipY - MarginY - Corner, RecordRed, 1.5f);
	}
}
