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
        if (!State->PrimaryResidenceID.IsEmpty())
        {
            DrawText(FString::Printf(TEXT("HOME  %s"), *State->CurrentAddress), FColor(178, 220, 195), Left, Top, Font, 0.85f);
            Top += LineHeight;
        }
        if (State->CurrentJob == EGCJob::Police)
        {
            const FColor ConductColor = State->ConductPoints <= 25 ? FColor::Red
                : (State->ConductPoints <= 60 ? FColor(255, 190, 70) : FColor(100, 220, 150));
            DrawText(FString::Printf(TEXT("BADGE  %s  |  %s"), *State->PoliceBadgeNumber, *State->CitizenName),
                FColor(174, 207, 255), Left, Top, Font, 0.9f);
            Top += LineHeight;
            DrawText(FString::Printf(TEXT("POLICE SHIFT  CP %d  /  SP %d%s"), State->ConductPoints, State->ShiftPoints,
                State->bPoliceShiftFailed ? TEXT("  SHIFT FAILED") : (State->bPoliceShiftActive ? TEXT("  ON DUTY") : TEXT("  OFF DUTY"))),
                ConductColor, Left, Top, Font, 0.95f);
            Top += LineHeight;
        }
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

        const bool bEngineWarning = Vehicle->EngineTemperatureC >= Vehicle->EngineOverheatTemperatureC;
        const FColor EngineColor = Vehicle->bEngineStalled ? FColor::Red
            : (bEngineWarning ? FColor(255, 170, 55) : FColor(155, 220, 185));
        DrawText(FString::Printf(TEXT("ENGINE  %3.0f C  RAD %d%%"), Vehicle->EngineTemperatureC,
            FMath::RoundToInt(Vehicle->RadiatorIntegrity * 100.0f)), EngineColor, Left, Top, Font, 0.9f);
        Top += LineHeight;
        if (Vehicle->bEngineStalled)
        {
            DrawText(TEXT("ENGINE STALLED — LET IT COOL"), FColor::Red, Left, Top, Font, 0.95f);
            Top += LineHeight;
        }
        for (int32 WheelIndex = 0; WheelIndex < Vehicle->TireConditions.Num(); ++WheelIndex)
        {
            const FGCVehicleTireCondition& Tire = Vehicle->TireConditions[WheelIndex];
            if (Tire.bPunctured || Tire.Integrity < 0.55f)
            {
                DrawText(FString::Printf(TEXT("TIRE %d  %s  %d%% GRIP"), WheelIndex + 1,
                    Tire.bPunctured ? TEXT("PUNCTURE") : TEXT("DAMAGED"),
                    FMath::RoundToInt(Tire.GripScale * 100.0f)),
                    Tire.bPunctured ? FColor::Red : FColor(255, 170, 55), Left, Top, Font, 0.9f);
                Top += LineHeight;
            }
        }
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
