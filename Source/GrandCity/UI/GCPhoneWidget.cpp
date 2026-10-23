#include "UI/GCPhoneWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor PhoneInk(0.012f, 0.025f, 0.040f, 1.0f);
	const FLinearColor PhonePanel(0.035f, 0.075f, 0.095f, 1.0f);
	const FLinearColor PhoneAccent(0.26f, 0.86f, 0.73f, 1.0f);
	const FLinearColor PhoneMuted(0.65f, 0.78f, 0.82f, 1.0f);
}

void UGCPhoneWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;
	PhoneFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	PhoneFrame->SetBrushColor(PhoneInk);
	PhoneFrame->SetPadding(FMargin(12.0f));
	if (UCanvasPanelSlot* PhoneSlot = Canvas->AddChildToCanvas(PhoneFrame))
	{
		PhoneSlot->SetAnchors(FAnchors(1.0f, 1.0f, 1.0f, 1.0f));
		PhoneSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		PhoneSlot->SetPosition(FVector2D(-28.0f, -24.0f));
		PhoneSlot->SetSize(FVector2D(350.0f, 590.0f));
	}

	UBorder* Screen = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Screen->SetBrushColor(PhonePanel);
	Screen->SetPadding(FMargin(18.0f));
	PhoneFrame->AddChild(Screen);
	UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Screen->AddChild(Layout);

	UTextBlock* StatusBar = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StatusBar->SetText(FText::FromString(TEXT("GRAND CITY MOBILE  /  FULL SIGNAL")));
	StatusBar->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 9));
	StatusBar->SetColorAndOpacity(FSlateColor(PhoneMuted));
	Layout->AddChildToVerticalBox(StatusBar);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("YOUR CITY, IN HAND")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 22));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UVerticalBoxSlot* TitleSlot = Layout->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 18.0f));
	}

	UTextBlock* WalletEyebrow = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	WalletEyebrow->SetText(FText::FromString(TEXT("WALLET")));
	WalletEyebrow->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	WalletEyebrow->SetColorAndOpacity(FSlateColor(PhoneAccent));
	Layout->AddChildToVerticalBox(WalletEyebrow);
	WalletText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	WalletText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 26));
	WalletText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UVerticalBoxSlot* WalletSlot = Layout->AddChildToVerticalBox(WalletText))
	{
		WalletSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 22.0f));
	}

	UTextBlock* TravelEyebrow = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TravelEyebrow->SetText(FText::FromString(TEXT("TRAVEL  /  MY BOARDING PASS")));
	TravelEyebrow->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	TravelEyebrow->SetColorAndOpacity(FSlateColor(PhoneAccent));
	Layout->AddChildToVerticalBox(TravelEyebrow);
	TravelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TravelText->SetAutoWrapText(true);
	TravelText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 15));
	TravelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UVerticalBoxSlot* TravelSlot = Layout->AddChildToVerticalBox(TravelText))
	{
		TravelSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
	}

	UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Hint->SetText(FText::FromString(TEXT("P  /  PUT PHONE AWAY")));
	Hint->SetJustification(ETextJustify::Center);
	Hint->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	Hint->SetColorAndOpacity(FSlateColor(PhoneMuted));
	if (UVerticalBoxSlot* HintSlot = Layout->AddChildToVerticalBox(Hint))
	{
		HintSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
	}

	RefreshPhoneText();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UGCPhoneWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	OpenProgress = FMath::FInterpTo(OpenProgress, bWantsOpen ? 1.0f : 0.0f, InDeltaTime, 11.0f);
	if (PhoneFrame)
	{
		PhoneFrame->SetRenderTranslation(FVector2D(390.0f * (1.0f - OpenProgress), 18.0f * (1.0f - OpenProgress)));
		PhoneFrame->SetRenderOpacity(OpenProgress);
	}
	if (!bWantsOpen && OpenProgress < 0.015f)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGCPhoneWidget::SetPhoneOpen(bool bOpen)
{
	bWantsOpen = bOpen;
	if (bOpen)
	{
		SetVisibility(ESlateVisibility::Visible);
	}
}

void UGCPhoneWidget::SetPhoneData(int32 WalletBalance, const FString& TravelSummary)
{
	CachedWalletBalance = WalletBalance;
	CachedTravelSummary = TravelSummary;
	RefreshPhoneText();
}

void UGCPhoneWidget::RefreshPhoneText()
{
	if (WalletText)
	{
		WalletText->SetText(FText::FromString(FString::Printf(TEXT("$ %d"), CachedWalletBalance)));
	}
	if (TravelText)
	{
		const FString Summary = CachedTravelSummary.IsEmpty()
			? FString(TEXT("No trip booked. Visit an airport kiosk to explore departures.")) : CachedTravelSummary;
		TravelText->SetText(FText::FromString(Summary));
	}
}
