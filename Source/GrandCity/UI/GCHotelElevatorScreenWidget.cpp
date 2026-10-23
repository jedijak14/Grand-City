#include "UI/GCHotelElevatorScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor ScreenInk(0.018f, 0.035f, 0.055f, 1.0f);
	const FLinearColor ScreenAccent(0.32f, 0.88f, 0.82f, 1.0f);
	const FLinearColor ScreenMuted(0.72f, 0.82f, 0.85f, 1.0f);

	FString FloorLabel(int32 Floor)
	{
		return Floor <= 0 ? FString::Printf(TEXT("B%d"), 1 - Floor) : FString::Printf(TEXT("%02d"), Floor);
	}
}

UGCHotelElevatorScreenWidget::UGCHotelElevatorScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	HotelAdAssets = {
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/HotelRewards/T_TidehouseHotels.T_TidehouseHotels"))),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/HotelRewards/T_MeridianHouse.T_MeridianHouse"))),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/HotelRewards/T_AstaraGrand.T_AstaraGrand")))
	};
}

void UGCHotelElevatorScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree)
	{
		return;
	}
	HotelAdSlides.Reset();
	for (const TSoftObjectPtr<UTexture2D>& AdAsset : HotelAdAssets)
	{
		HotelAdSlides.Add(AdAsset.LoadSynchronous());
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Frame->SetBrushColor(ScreenInk);
	Frame->SetPadding(FMargin(18.0f));
	if (UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Frame))
	{
		FrameSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		FrameSlot->SetOffsets(FMargin(0.0f));
	}
	UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Frame->AddChild(Layout);

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* Brand = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Brand->SetText(FText::FromString(TEXT("GRIDLOCK  /  STAYS")));
	Brand->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 18));
	Brand->SetColorAndOpacity(FSlateColor(ScreenAccent));
	if (UHorizontalBoxSlot* BrandSlot = Header->AddChildToHorizontalBox(Brand))
	{
		BrandSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	FloorReadout = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FloorReadout->SetJustification(ETextJustify::Right);
	FloorReadout->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 24));
	FloorReadout->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Header->AddChildToHorizontalBox(FloorReadout);
	if (UVerticalBoxSlot* HeaderSlot = Layout->AddChildToVerticalBox(Header))
	{
		HeaderSlot->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 6.0f));
	}

	UHorizontalBox* FloorButtons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UBorder* Button = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Button->SetBrushColor(FLinearColor(0.07f, 0.16f, 0.19f, 1.0f));
		Button->SetPadding(FMargin(7.0f, 4.0f));
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetJustification(ETextJustify::Center);
		Label->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Button->AddChild(Label);
		FloorButtonFrames.Add(Button);
		FloorButtonLabels.Add(Label);
		if (UHorizontalBoxSlot* Slot = FloorButtons->AddChildToHorizontalBox(Button))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetPadding(FMargin(3.0f, 0.0f));
		}
	}
	if (UVerticalBoxSlot* ButtonsSlot = Layout->AddChildToVerticalBox(FloorButtons))
	{
		ButtonsSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	UBorder* AdFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	AdFrame->SetBrushColor(FLinearColor::White);
	AdFrame->SetPadding(FMargin(6.0f));
	USizeBox* AdSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	AdSize->SetWidthOverride(640.0f);
	AdSize->SetHeightOverride(280.0f);
	UOverlay* AdOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	HotelFallback = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HotelFallback->SetText(FText::FromString(TEXT("FEATURED HOTEL")));
	HotelFallback->SetJustification(ETextJustify::Center);
	HotelFallback->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 28));
	HotelFallback->SetColorAndOpacity(FSlateColor(ScreenInk));
	AdOverlay->AddChild(HotelFallback);
	UScaleBox* AdScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
	AdScale->SetStretch(EStretch::ScaleToFit);
	HotelAdImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	AdScale->AddChild(HotelAdImage);
	AdOverlay->AddChild(AdScale);
	AdSize->AddChild(AdOverlay);
	AdFrame->AddChild(AdSize);
	if (UVerticalBoxSlot* AdSlot = Layout->AddChildToVerticalBox(AdFrame))
	{
		AdSlot->SetHorizontalAlignment(HAlign_Center);
		AdSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	UTextBlock* Footer = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Footer->SetText(FText::FromString(TEXT("FEATURED HOTEL  /  ROTATING CAMPAIGN")));
	Footer->SetJustification(ETextJustify::Center);
	Footer->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	Footer->SetColorAndOpacity(FSlateColor(ScreenMuted));
	Layout->AddChildToVerticalBox(Footer);

	SetElevatorReadout(1, false, 1);
	RefreshFloorReadout();
	if (FloorButtonLabels.IsValidIndex(0) && FloorButtonLabels.IsValidIndex(1))
	{
		FloorButtonLabels[0]->SetText(FText::FromString(TEXT("CURRENT FLOOR 01")));
		FloorButtonLabels[1]->SetText(FText::FromString(TEXT("SELECT DESTINATION")));
	}
	AdvanceAdSlide();
}

void UGCHotelElevatorScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (HotelAdAssets.Num() >= 2)
	{
		AdSlideTime += InDeltaTime;
		if (AdSlideTime >= 7.0f)
		{
			AdSlideTime = FMath::Fmod(AdSlideTime, 7.0f);
			AdvanceAdSlide();
		}
	}
	ButtonPressRemaining = FMath::Max(0.0f, ButtonPressRemaining - InDeltaTime);
	for (int32 Index = 0; Index < FloorButtonFrames.Num(); ++Index)
	{
		UBorder* Button = FloorButtonFrames[Index];
		if (!Button)
		{
			continue;
		}
		const bool bPressed = Index == PressedFloorButtonIndex && ButtonPressRemaining > 0.0f;
		const float Phase = bPressed ? 1.0f - ButtonPressRemaining / 0.42f : 1.0f;
		const float Pulse = bPressed ? FMath::Sin(Phase * PI) : 0.0f;
		const float Scale = 1.0f + 0.14f * Pulse;
		Button->SetRenderScale(FVector2D(Scale, Scale));
		Button->SetBrushColor(FMath::Lerp(FLinearColor(0.07f, 0.16f, 0.19f, 1.0f),
			FLinearColor(0.10f, 0.72f, 0.62f, 1.0f), Pulse));
	}
}

void UGCHotelElevatorScreenWidget::SetElevatorReadout(int32 CurrentFloor, bool bMoving, int32 DestinationFloor)
{
	if (CachedCurrentFloor == CurrentFloor && CachedDestinationFloor == DestinationFloor && bCachedMoving == bMoving)
	{
		return;
	}
	CachedCurrentFloor = CurrentFloor;
	CachedDestinationFloor = DestinationFloor;
	bCachedMoving = bMoving;
	RefreshFloorReadout();
}

void UGCHotelElevatorScreenWidget::AnimateFloorSelection(int32 CurrentFloor, int32 DestinationFloor)
{
	CachedFloorButtonCurrent = CurrentFloor;
	CachedFloorButtonDestination = DestinationFloor;
	if (FloorButtonLabels.IsValidIndex(0))
	{
		FloorButtonLabels[0]->SetText(FText::FromString(FString::Printf(TEXT("CURRENT FLOOR %s"), *FloorLabel(CurrentFloor))));
	}
	if (FloorButtonLabels.IsValidIndex(1))
	{
		FloorButtonLabels[1]->SetText(FText::FromString(FString::Printf(TEXT("SELECT FLOOR %s"), *FloorLabel(DestinationFloor))));
	}
	PressedFloorButtonIndex = 1;
	ButtonPressRemaining = 0.42f;
}

void UGCHotelElevatorScreenWidget::SetIdleFloorButtonLabel(int32 CurrentFloor)
{
	if (CachedFloorButtonCurrent == CurrentFloor && CachedFloorButtonDestination == INDEX_NONE)
	{
		return;
	}
	CachedFloorButtonCurrent = CurrentFloor;
	CachedFloorButtonDestination = INDEX_NONE;
	if (FloorButtonLabels.IsValidIndex(0))
	{
		FloorButtonLabels[0]->SetText(FText::FromString(FString::Printf(TEXT("CURRENT FLOOR %s"), *FloorLabel(CurrentFloor))));
	}
	if (FloorButtonLabels.IsValidIndex(1))
	{
		FloorButtonLabels[1]->SetText(FText::FromString(TEXT("SELECT DESTINATION")));
	}
}

void UGCHotelElevatorScreenWidget::RefreshFloorReadout()
{
	if (!FloorReadout)
	{
		return;
	}
	FString Text = FString::Printf(TEXT("FLOOR %s"), *FloorLabel(CachedCurrentFloor));
	if (bCachedMoving)
	{
		const TCHAR* Direction = CachedDestinationFloor >= CachedCurrentFloor ? TEXT("UP TO") : TEXT("DOWN TO");
		Text = FString::Printf(TEXT("%s %s"), Direction, *FloorLabel(CachedDestinationFloor));
	}
	FloorReadout->SetText(FText::FromString(Text));
}

void UGCHotelElevatorScreenWidget::AdvanceAdSlide()
{
	static const TCHAR* HotelNames[] = { TEXT("TIDEHOUSE HOTELS"), TEXT("MERIDIAN HOUSE"), TEXT("ASTARA GRAND") };
	ActiveAdIndex = (ActiveAdIndex + 1) % UE_ARRAY_COUNT(HotelNames);
	if (HotelAdSlides.IsValidIndex(ActiveAdIndex) && HotelAdSlides[ActiveAdIndex])
	{
		HotelAdImage->SetBrushFromTexture(HotelAdSlides[ActiveAdIndex], false);
		HotelAdImage->SetVisibility(ESlateVisibility::Visible);
		HotelFallback->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		HotelAdImage->SetVisibility(ESlateVisibility::Collapsed);
		HotelFallback->SetText(FText::FromString(HotelNames[ActiveAdIndex]));
		HotelFallback->SetVisibility(ESlateVisibility::Visible);
	}
}
