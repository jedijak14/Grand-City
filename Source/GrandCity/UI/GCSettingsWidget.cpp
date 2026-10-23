#include "UI/GCSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/GCGameUserSettings.h"
#include "Character/GCCharacter.h"
#include "Character/GCPedestrian.h"
#include "Components/GCClothPhysicsLibrary.h"
#include "EngineUtils.h"

namespace
{
	void AddSettingsLabel(UWidgetTree* Tree, UVerticalBox* Parent, const TCHAR* Label)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Label));
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Text))
		{
			Slot->SetPadding(FMargin(8.0f, 4.0f));
		}
	}

	UCheckBox* AddSettingsCheckBox(UWidgetTree* Tree, UVerticalBox* Parent, const TCHAR* Label)
	{
		UCheckBox* CheckBox = Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Label));
		CheckBox->AddChild(Text);
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(CheckBox))
		{
			Slot->SetPadding(FMargin(8.0f, 3.0f));
		}
		return CheckBox;
	}

	USlider* AddSettingsSlider(UWidgetTree* Tree, UVerticalBox* Parent, const TCHAR* Label)
	{
		AddSettingsLabel(Tree, Parent, Label);
		USlider* Slider = Tree->ConstructWidget<USlider>(USlider::StaticClass());
		Slider->SetMinValue(0.0f);
		Slider->SetMaxValue(1.0f);
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Slider))
		{
			Slot->SetPadding(FMargin(12.0f, 2.0f));
		}
		return Slider;
	}
}

void UGCSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;
	USizeBox* Panel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Panel->SetWidthOverride(560.0f);
	Panel->SetHeightOverride(720.0f);
	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->AddChild(Stack);
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	}

	AddSettingsLabel(WidgetTree, Stack, TEXT("GRIDLOCK SETTINGS"));
	BloodCheckBox = AddSettingsCheckBox(WidgetTree, Stack, TEXT("Blood effects"));
	WavesCheckBox = AddSettingsCheckBox(WidgetTree, Stack, TEXT("Ocean waves"));
	GrassCheckBox = AddSettingsCheckBox(WidgetTree, Stack, TEXT("Grass"));
	ClothCheckBox = AddSettingsCheckBox(WidgetTree, Stack, TEXT("Chaos Cloth / authored hair simulation"));
	TrafficDensitySlider = AddSettingsSlider(WidgetTree, Stack, TEXT("Traffic density (0–60)"));
	PedestrianDensitySlider = AddSettingsSlider(WidgetTree, Stack, TEXT("Pedestrian density (0–80)"));
	DayLengthSlider = AddSettingsSlider(WidgetTree, Stack, TEXT("Day length (0 = frozen, up to 30 minutes)"));
	BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* BackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BackText->SetText(FText::FromString(TEXT("BACK")));
	BackText->SetJustification(ETextJustify::Center);
	BackButton->AddChild(BackText);
	if (UVerticalBoxSlot* BackSlot = Stack->AddChildToVerticalBox(BackButton))
	{
		BackSlot->SetPadding(FMargin(8.0f, 16.0f));
	}

	BloodCheckBox->OnCheckStateChanged.AddDynamic(this, &UGCSettingsWidget::HandleSettingChanged);
	WavesCheckBox->OnCheckStateChanged.AddDynamic(this, &UGCSettingsWidget::HandleSettingChanged);
	GrassCheckBox->OnCheckStateChanged.AddDynamic(this, &UGCSettingsWidget::HandleSettingChanged);
	ClothCheckBox->OnCheckStateChanged.AddDynamic(this, &UGCSettingsWidget::HandleSettingChanged);
	TrafficDensitySlider->OnValueChanged.AddDynamic(this, &UGCSettingsWidget::HandleTrafficDensityChanged);
	PedestrianDensitySlider->OnValueChanged.AddDynamic(this, &UGCSettingsWidget::HandlePedestrianDensityChanged);
	DayLengthSlider->OnValueChanged.AddDynamic(this, &UGCSettingsWidget::HandleDayLengthChanged);
	BackButton->OnClicked.AddDynamic(this, &UGCSettingsWidget::HandleBackClicked);
	LoadFromSettings();
}

void UGCSettingsWidget::LoadFromSettings()
{
	const UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings();
	if (!Settings)
	{
		return;
	}
	bLoadingSettings = true;
	if (BloodCheckBox) { BloodCheckBox->SetIsChecked(Settings->bBloodEnabled); }
	if (WavesCheckBox) { WavesCheckBox->SetIsChecked(Settings->bWavesEnabled); }
	if (GrassCheckBox) { GrassCheckBox->SetIsChecked(Settings->bGrassEnabled); }
	if (ClothCheckBox) { ClothCheckBox->SetIsChecked(Settings->bClothAndHairPhysics); }
	if (TrafficDensitySlider) { TrafficDensitySlider->SetValue(FMath::Clamp(Settings->TrafficDensity / 60.0f, 0.0f, 1.0f)); }
	if (PedestrianDensitySlider) { PedestrianDensitySlider->SetValue(FMath::Clamp(Settings->PedestrianDensity / 80.0f, 0.0f, 1.0f)); }
	if (DayLengthSlider) { DayLengthSlider->SetValue(FMath::Clamp(Settings->DayLengthSeconds / 1800.0f, 0.0f, 1.0f)); }
	bLoadingSettings = false;
}

void UGCSettingsWidget::HandleSettingChanged(bool)
{
	if (bLoadingSettings)
	{
		return;
	}
	UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings();
	if (!Settings)
	{
		return;
	}
	Settings->bBloodEnabled = BloodCheckBox && BloodCheckBox->IsChecked();
	Settings->bWavesEnabled = WavesCheckBox && WavesCheckBox->IsChecked();
	Settings->bGrassEnabled = GrassCheckBox && GrassCheckBox->IsChecked();
    const bool bClothWasEnabled = Settings->bClothAndHairPhysics;
	Settings->bClothAndHairPhysics = ClothCheckBox && ClothCheckBox->IsChecked();
    if (bClothWasEnabled != Settings->bClothAndHairPhysics && GetWorld())
    {
        for (TActorIterator<AGCCharacter> It(GetWorld()); It; ++It)
        {
            It->RefreshClothAndHairVisibility();
        }
        for (TActorIterator<AGCPedestrian> It(GetWorld()); It; ++It)
        {
            UGCClothPhysicsLibrary::ApplyClothSimulation(*It, Settings->bClothAndHairPhysics, 1.0f);
        }
    }
	Settings->ApplySettings(false);
	Settings->SaveSettings();
}

void UGCSettingsWidget::HandleTrafficDensityChanged(float Value)
{
	if (!bLoadingSettings)
	{
		if (UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings())
		{
			Settings->TrafficDensity = FMath::RoundToInt(FMath::Clamp(Value, 0.0f, 1.0f) * 60.0f);
			Settings->SaveSettings();
		}
	}
}

void UGCSettingsWidget::HandlePedestrianDensityChanged(float Value)
{
	if (!bLoadingSettings)
	{
		if (UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings())
		{
			Settings->PedestrianDensity = FMath::RoundToInt(FMath::Clamp(Value, 0.0f, 1.0f) * 80.0f);
			Settings->SaveSettings();
		}
	}
}

void UGCSettingsWidget::HandleDayLengthChanged(float Value)
{
	if (!bLoadingSettings)
	{
		if (UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings())
		{
			Settings->DayLengthSeconds = static_cast<float>(FMath::RoundToInt(FMath::Clamp(Value, 0.0f, 1.0f) * 1800.0f));
			Settings->SaveSettings();
		}
	}
}

void UGCSettingsWidget::HandleBackClicked()
{
	OnClosed.Broadcast();
}
