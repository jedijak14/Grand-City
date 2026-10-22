#include "UI/GCLoadingScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor GCLoadingInk(0.018f, 0.040f, 0.065f, 1.0f);
	const FLinearColor GCLoadingPanel(0.025f, 0.075f, 0.105f, 0.96f);
	const FLinearColor GCLoadingAccent(0.14f, 0.82f, 0.72f, 1.0f);
	const FLinearColor GCLoadingMuted(0.62f, 0.76f, 0.80f, 1.0f);
}

void UGCLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;

	UImage* Backdrop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (BackgroundTexture)
	{
		Backdrop->SetBrushFromTexture(BackgroundTexture, false);
		Backdrop->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		Backdrop->SetColorAndOpacity(GCLoadingInk);
	}
	if (UCanvasPanelSlot* BackdropSlot = Canvas->AddChildToCanvas(Backdrop))
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetOffsets(FMargin(0.0f));
	}

	if (!BackgroundTexture)
	{
		// Lightweight native skyline silhouettes keep the loading screen self-contained (no texture asset required).
		auto AddBackdropShape = [this, Canvas](float X0, float Y0, float X1, float Y1, const FLinearColor& Color)
		{
			UBorder* Shape = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Shape->SetBrushColor(Color);
			if (UCanvasPanelSlot* ShapeSlot = Canvas->AddChildToCanvas(Shape))
			{
				ShapeSlot->SetAnchors(FAnchors(X0, Y0, X1, Y1));
				ShapeSlot->SetOffsets(FMargin(0.0f));
			}
		};
		AddBackdropShape(0.0f, 0.67f, 1.0f, 0.91f, FLinearColor(0.012f, 0.042f, 0.060f, 1.0f));
		const float Skyline[][3] = {
			{ 0.00f, 0.53f, 0.07f }, { 0.08f, 0.49f, 0.05f }, { 0.15f, 0.56f, 0.08f },
			{ 0.25f, 0.46f, 0.06f }, { 0.33f, 0.51f, 0.09f }, { 0.44f, 0.44f, 0.07f },
			{ 0.53f, 0.50f, 0.08f }, { 0.63f, 0.42f, 0.06f }, { 0.71f, 0.48f, 0.08f },
			{ 0.81f, 0.43f, 0.07f }, { 0.90f, 0.49f, 0.06f }, { 0.97f, 0.54f, 0.03f }
		};
		for (const float (&Building)[3] : Skyline)
		{
			AddBackdropShape(Building[0], Building[1], Building[0] + Building[2], 0.70f,
				FLinearColor(0.025f, 0.075f, 0.095f, 1.0f));
		}
		AddBackdropShape(0.0f, 0.91f, 1.0f, 1.0f, FLinearColor(0.009f, 0.028f, 0.045f, 1.0f));
		for (int32 Reflection = 0; Reflection < 11; ++Reflection)
		{
			const float X = 0.36f + 0.047f * Reflection;
			const float Y = 0.75f + 0.016f * (Reflection % 4);
			AddBackdropShape(X, Y, X + 0.09f + 0.012f * (Reflection % 3), Y + 0.0025f,
				FLinearColor(0.07f, 0.24f, 0.27f, 0.72f));
		}
	}

	UVerticalBox* BrandStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* Brand = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Brand->SetText(FText::FromString(TEXT("GRAND  /  CITY")));
	Brand->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 28));
	Brand->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	BrandStack->AddChildToVerticalBox(Brand);
	UTextBlock* Edition = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Edition->SetText(FText::FromString(TEXT("PACIFIC COAST  •  OPEN-WORLD STORY")));
	Edition->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
	Edition->SetColorAndOpacity(FSlateColor(GCLoadingMuted));
	BrandStack->AddChildToVerticalBox(Edition);
	if (UCanvasPanelSlot* BrandSlot = Canvas->AddChildToCanvas(BrandStack))
	{
		BrandSlot->SetAnchors(FAnchors(0.065f, 0.075f, 0.065f, 0.075f));
		BrandSlot->SetPosition(FVector2D(0.0f, 0.0f));
	}

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Card->SetBrushColor(GCLoadingPanel);
	Card->SetPadding(FMargin(34.0f, 28.0f));
	USizeBox* CardWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CardWidth->SetWidthOverride(760.0f);
	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	CardWidth->AddChild(Stack);
	Card->AddChild(CardWidth);

	UTextBlock* Eyebrow = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Eyebrow->SetText(FText::FromString(TEXT("WORLD STREAMING  /  SAN DIEGO")));
	Eyebrow->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
	Eyebrow->SetColorAndOpacity(FSlateColor(GCLoadingAccent));
	if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(Eyebrow))
	{
		Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("A CITY IN MOTION")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 36));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(Title))
	{
		Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 5.0f));
	}

	UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Subtitle->SetText(FText::FromString(TEXT("Stitching together streets, shoreline and stories.")));
	Subtitle->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 16));
	Subtitle->SetColorAndOpacity(FSlateColor(GCLoadingMuted));
	if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(Subtitle))
	{
		Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
	}

	UHorizontalBox* StatusRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* Status = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Status->SetText(FText::FromString(TEXT("PREPARING THE COASTLINE")));
	Status->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	StatusRow->AddChildToHorizontalBox(Status);
	ProgressLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ProgressLabel->SetText(FText::FromString(TEXT("0%")));
	ProgressLabel->SetColorAndOpacity(FSlateColor(GCLoadingAccent));
	if (UHorizontalBoxSlot* Slot = StatusRow->AddChildToHorizontalBox(ProgressLabel))
	{
		Slot->SetHorizontalAlignment(HAlign_Right);
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(StatusRow))
	{
		Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 9.0f));
	}

	ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	ProgressBar->SetFillColorAndOpacity(GCLoadingAccent);
	Stack->AddChildToVerticalBox(ProgressBar);

	UTextBlock* Tip = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Tip->SetText(FText::FromString(TEXT("TIP   •   Take the coast road at sunrise for the quietest drive.")));
	Tip->SetColorAndOpacity(FSlateColor(GCLoadingMuted));
	if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(Tip))
	{
		Slot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
	}

	if (UCanvasPanelSlot* CardSlot = Canvas->AddChildToCanvas(Card))
	{
		CardSlot->SetAnchors(FAnchors(0.5f, 0.78f, 0.5f, 0.78f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	}
	SetProgress(0.0f);
}

void UGCLoadingScreenWidget::SetProgress(float Progress)
{
	const float SafeProgress = FMath::Clamp(FMath::IsFinite(Progress) ? Progress : 0.0f, 0.0f, 1.0f);
	if (ProgressBar)
	{
		ProgressBar->SetPercent(SafeProgress);
	}
	if (ProgressLabel)
	{
		ProgressLabel->SetText(FText::AsPercent(SafeProgress, 0));
	}
}
