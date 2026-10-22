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
		DrawText(FString::Printf(TEXT("$ %d"), State->Money), FColor::White, Left, Top, Font, 1.15f);
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
}
