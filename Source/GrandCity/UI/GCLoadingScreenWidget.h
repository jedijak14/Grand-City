#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GCLoadingScreenWidget.generated.h"

class UProgressBar;
class UTexture2D;

/** Native loading overlay with a controllable progress bar. */
UCLASS()
class GRANDCITY_API UGCLoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Optional imported loading-key-art texture. If unset, the native skyline fallback is drawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loading|Art")
	TObjectPtr<UTexture2D> BackgroundTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
	bool bSimulateProgress = true;

	UFUNCTION(BlueprintCallable, Category = "Loading")
	void SetProgress(float Progress);

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> ProgressLabel;
};
