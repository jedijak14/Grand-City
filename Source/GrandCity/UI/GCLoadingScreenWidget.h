#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GCLoadingScreenWidget.generated.h"

class UProgressBar;

/** Native loading overlay with a controllable progress bar. */
UCLASS()
class GRANDCITY_API UGCLoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
	bool bSimulateProgress = true;

	UFUNCTION(BlueprintCallable, Category = "Loading")
	void SetProgress(float Progress);

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ProgressBar;
};
