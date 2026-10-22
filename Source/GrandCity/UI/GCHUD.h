#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GCHUD.generated.h"

/** Minimal native HUD base. Gameplay widgets are owned by the player controller. */
UCLASS()
class GRANDCITY_API AGCHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
