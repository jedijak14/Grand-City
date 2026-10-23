#include "UI/GCHotelElevatorScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
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
		return Floor <= 0
			? FString::Printf(TEXT("B%d"), 1 - Floor)
			: FString::Printf(TEXT("%02d"), Floor);
	}
}

UGCHotelElevatorScreenWidget::UGCHotelElevatorScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PartnerLogoAssets = {
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/HotelRewards/T_HiltonHonors.T_HiltonHonors"))),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/HotelRewards/T_MarriottBonvoy.T_MarriottBonvoy"))),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/HotelRewards/T_IHGOneRewards.T_IHGOneRewards")))
	};
}

void UGCHotelElevatorScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree)
	{
		return;
	}

	PartnerLogos.Reset();
	for (const TSoftObjectPtr<UTexture2D>& LogoAsset : PartnerLogoAssets)
	{
		PartnerLogos.Add(LogoAsset.LoadSynchronous());
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
	Brand->SetText(FText::FromString(TEXT("GRAND CITY  /  HOTEL")));
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
		HeaderSlot->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 12.0f));
	}

	UBorder* LogoFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	LogoFrame->SetBrushColor(FLinearColor::White);
	LogoFrame->SetPadding(FMargin(8.0f));
	USizeBox* LogoSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LogoSize->SetWidthOverride(640.0f);
	LogoSize->SetHeightOverride(255.0f);
	UScaleBox* LogoScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
	LogoScale->SetStretch(EStretch::ScaleToFit);
	PartnerLogoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	PartnerLogoImage->SetColorAndOpacity(FLinearColor::White);
	LogoScale->AddChild(PartnerLogoImage);
	LogoSize->AddChild(LogoScale);
	LogoFrame->AddChild(LogoSize);
	if (UVerticalBoxSlot* LogoSlot = Layout->AddChildToVerticalBox(LogoFrame))
	{
		LogoSlot->SetHorizontalAlignment(HAlign_Center);
		LogoSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	UTextBlock* RewardsEyebrow = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RewardsEyebrow->SetText(FText::FromString(TEXT("HOTEL REWARDS PARTNERS")));
	RewardsEyebrow->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	RewardsEyebrow->SetColorAndOpacity(FSlateColor(ScreenMuted));
	if (UVerticalBoxSlot* EyebrowSlot = Layout->AddChildToVerticalBox(RewardsEyebrow))
	{
		EyebrowSlot->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 2.0f));
	}

	PartnerName = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PartnerName->SetJustification(ETextJustify::Center);
	PartnerName->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 20));
	PartnerName->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UVerticalBoxSlot* PartnerSlot = Layout->AddChildToVerticalBox(PartnerName))
	{
		PartnerSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UTextBlock* Footer = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Footer->SetText(FText::FromString(TEXT("Discover member benefits at participating stays.")));
	Footer->SetJustification(ETextJustify::Center);
	Footer->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	Footer->SetColorAndOpacity(FSlateColor(ScreenMuted));
	if (UVerticalBoxSlot* FooterSlot = Layout->AddChildToVerticalBox(Footer))
	{
		FooterSlot->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 0.0f));
	}

	SetElevatorReadout(1, false, 1);
	RefreshFloorReadout();
	AdvancePartnerSlide();
}

void UGCHotelElevatorScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (PartnerLogoAssets.Num() < 2)
	{
		return;
	}

	PartnerSlideTime += InDeltaTime;
	if (PartnerSlideTime >= 7.0f)
	{
		PartnerSlideTime = FMath::Fmod(PartnerSlideTime, 7.0f);
		AdvancePartnerSlide();
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

void UGCHotelElevatorScreenWidget::AdvancePartnerSlide()
{
	static const TCHAR* PartnerLabels[] = {
		TEXT("Hilton Honors"),
		TEXT("Marriott Bonvoy"),
		TEXT("IHG One Rewards")
	};
	const int32 PartnerCount = UE_ARRAY_COUNT(PartnerLabels);
	ActivePartnerIndex = (ActivePartnerIndex + 1) % PartnerCount;
	if (PartnerName)
	{
		PartnerName->SetText(FText::FromString(PartnerLabels[ActivePartnerIndex]));
	}
	if (PartnerLogoImage && PartnerLogos.IsValidIndex(ActivePartnerIndex))
	{
		if (UTexture2D* Logo = PartnerLogos[ActivePartnerIndex])
		{
			PartnerLogoImage->SetBrushFromTexture(Logo, true);
			PartnerLogoImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			PartnerLogoImage->SetBrush(FSlateBrush());
			PartnerLogoImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
