#include "Core/GCGameUserSettings.h"

UGCGameUserSettings::UGCGameUserSettings()
{
}

UGCGameUserSettings* UGCGameUserSettings::GetGCSettings()
{
	return Cast<UGCGameUserSettings>(UGameUserSettings::GetGameUserSettings());
}

void UGCGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	Super::ApplySettings(bCheckForCommandLineOverrides);
	SaveSettings();
}
