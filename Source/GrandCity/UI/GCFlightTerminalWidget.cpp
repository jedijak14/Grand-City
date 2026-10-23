#include "UI/GCFlightTerminalWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Character/GCCharacter.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "World/GCComputerTerminalActor.h"

namespace
{
	const FLinearColor KioskInk(0.015f, 0.035f, 0.055f, 0.96f);
	const FLinearColor KioskPanel(0.035f, 0.085f, 0.105f, 1.0f);
	const FLinearColor KioskAccent(0.20f, 0.86f, 0.73f, 1.0f);
	const FLinearColor KioskMuted(0.67f, 0.79f, 0.82f, 1.0f);
}

void UGCFlightTerminalWidget::SetTerminal(AGCComputerTerminalActor* InTerminal, AGCCharacter* InUser)
{
	Terminal = InTerminal;
	User = InUser;
	RefreshScreen(true);
}

void UGCFlightTerminalWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Panel->SetBrushColor(KioskInk);
	Panel->SetPadding(FMargin(24.0f));
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.035f, 0.16f, 0.035f, 0.16f));
		PanelSlot->SetPosition(FVector2D(0.0f, 0.0f));
		PanelSlot->SetSize(FVector2D(620.0f, 560.0f));
	}

	UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->AddChild(Layout);
	AirportTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	AirportTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 22));
	AirportTitle->SetColorAndOpacity(FSlateColor(KioskAccent));
	if (UVerticalBoxSlot* TitleSlot = Layout->AddChildToVerticalBox(AirportTitle))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	UTextBlock* Eyebrow = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Eyebrow->SetText(FText::FromString(TEXT("LIVE DEPARTURES  /  SELECT A FLIGHT")));
	Eyebrow->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	Eyebrow->SetColorAndOpacity(FSlateColor(KioskMuted));
	if (UVerticalBoxSlot* EyebrowSlot = Layout->AddChildToVerticalBox(Eyebrow))
	{
		EyebrowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	SelectionFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	SelectionFrame->SetBrushColor(KioskPanel);
	SelectionFrame->SetPadding(FMargin(16.0f));
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	FlightScreenText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FlightScreenText->SetAutoWrapText(true);
	FlightScreenText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14));
	FlightScreenText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Scroll->AddChild(FlightScreenText);
	SelectionFrame->AddChild(Scroll);
	if (UVerticalBoxSlot* ScreenSlot = Layout->AddChildToVerticalBox(SelectionFrame))
	{
		ScreenSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ScreenSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	UHorizontalBox* KeyRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const TCHAR* KeyHints[] = { TEXT("TAB   FLIGHT"), TEXT("B   SEAT CLASS"), TEXT("E   SELECT / BOOK") };
	for (const TCHAR* Hint : KeyHints)
	{
		UBorder* Key = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Key->SetBrushColor(FLinearColor(0.07f, 0.16f, 0.19f, 1.0f));
		Key->SetPadding(FMargin(10.0f, 7.0f));
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Hint));
		Text->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Key->AddChild(Text);
		if (UHorizontalBoxSlot* KeySlot = KeyRow->AddChildToHorizontalBox(Key))
		{
			KeySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			KeySlot->SetPadding(FMargin(3.0f, 0.0f));
		}
	}
	if (UVerticalBoxSlot* KeysSlot = Layout->AddChildToVerticalBox(KeyRow))
	{
		KeysSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	ActionFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	ActionFrame->SetBrushColor(KioskAccent);
	ActionFrame->SetPadding(FMargin(12.0f, 8.0f));
	ActionLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ActionLabel->SetText(FText::FromString(TEXT("E   BOOK FLIGHT / CHECK IN")));
	ActionLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 15));
	ActionLabel->SetJustification(ETextJustify::Center);
	ActionLabel->SetColorAndOpacity(FSlateColor(KioskInk));
	ActionFrame->AddChild(ActionLabel);
	if (UVerticalBoxSlot* ActionSlot = Layout->AddChildToVerticalBox(ActionFrame))
	{
		ActionSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	CachedScreenText.Empty();
	CachedOfferIndex = INDEX_NONE;
	RefreshScreen(true);
}

void UGCFlightTerminalWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshScreen(true);

	SelectionPulseRemaining = FMath::Max(0.0f, SelectionPulseRemaining - InDeltaTime);
	if (SelectionFrame)
	{
		const float Phase = SelectionPulseRemaining > 0.0f ? 1.0f - SelectionPulseRemaining / 0.38f : 1.0f;
		const float Pulse = SelectionPulseRemaining > 0.0f ? FMath::Sin(Phase * PI) : 0.0f;
		SelectionFrame->SetRenderScale(FVector2D(1.0f + 0.025f * Pulse));
		SelectionFrame->SetBrushColor(FMath::Lerp(KioskPanel, FLinearColor(0.04f, 0.23f, 0.24f, 1.0f), Pulse));
	}

	ActionPulseRemaining = FMath::Max(0.0f, ActionPulseRemaining - InDeltaTime);
	if (ActionFrame)
	{
		const float Phase = ActionPulseRemaining > 0.0f ? 1.0f - ActionPulseRemaining / 0.42f : 1.0f;
		const float Pulse = ActionPulseRemaining > 0.0f ? FMath::Sin(Phase * PI) : 0.0f;
		ActionFrame->SetRenderScale(FVector2D(1.0f + 0.12f * Pulse));
		ActionFrame->SetBrushColor(FMath::Lerp(KioskAccent, FLinearColor::White, Pulse));
	}
}

void UGCFlightTerminalWidget::PlaySelectionFeedback()
{
	SelectionPulseRemaining = 0.38f;
	RefreshScreen(false);
}

void UGCFlightTerminalWidget::PlayActionFeedback()
{
	ActionPulseRemaining = 0.42f;
}

void UGCFlightTerminalWidget::RefreshScreen(bool bCheckSelection)
{
	AGCComputerTerminalActor* ActiveTerminal = Terminal.Get();
	AGCCharacter* ActiveUser = User.Get();
	if (!ActiveTerminal)
	{
		return;
	}

	if (AirportTitle)
	{
		AirportTitle->SetText(FText::FromString(FString::Printf(TEXT("%s  /  %s"),
			*ActiveTerminal->AirportCity, *ActiveTerminal->AirportCode)));
	}
	const FString NewScreenText = ActiveTerminal->GetScreenText(ActiveUser);
	if (FlightScreenText && NewScreenText != CachedScreenText)
	{
		FlightScreenText->SetText(FText::FromString(NewScreenText));
		CachedScreenText = NewScreenText;
	}

	if (bCheckSelection)
	{
		const uint8 SeatClass = static_cast<uint8>(ActiveTerminal->SelectedClass);
		if (CachedOfferIndex != INDEX_NONE && (CachedOfferIndex != ActiveTerminal->SelectedOfferIndex || CachedSeatClass != SeatClass))
		{
			SelectionPulseRemaining = 0.38f;
		}
		CachedOfferIndex = ActiveTerminal->SelectedOfferIndex;
		CachedSeatClass = SeatClass;
	}
}
