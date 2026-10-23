#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GCIdentityCardWidget.generated.h"

class UTexture2D;
class UImage;

/** Runtime-generated citizen ID card with a procedurally rasterized portrait panel. */
UCLASS()
class GRANDCITY_API UGCIdentityCardWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> PortraitTexture = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UImage> PortraitImage = nullptr;

    UTexture2D* GeneratePortrait(const FString& SeedName);
};
