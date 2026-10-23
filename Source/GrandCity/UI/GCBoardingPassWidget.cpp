#include "UI/GCBoardingPassWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

void UGCBoardingPassWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Rebuild();
}

void UGCBoardingPassWidget::ConfigurePass(const FGCFlightTicket& InTicket, bool bInPaperCopy)
{
    Ticket = InTicket;
    bPaperCopy = bInPaperCopy;
    bConfigured = true;
    if (bBuilt)
    {
        Rebuild();
    }
}

void UGCBoardingPassWidget::Rebuild()
{
    if (!WidgetTree)
    {
        return;
    }
    WidgetTree->RootWidget = nullptr;
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    WidgetTree->RootWidget = Canvas;

    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Backdrop->SetBrushColor(FLinearColor(0.01f, 0.018f, 0.025f, 0.76f));
    if (UCanvasPanelSlot* BackdropSlot = Canvas->AddChildToCanvas(Backdrop))
    {
        BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        BackdropSlot->SetOffsets(FMargin(0.0f));
    }

    UBorder* Pass = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Pass->SetPadding(FMargin(bPaperCopy ? 32.0f : 24.0f));
    Pass->SetBrushColor(bPaperCopy ? FLinearColor(0.94f, 0.92f, 0.84f, 1.0f)
                                   : FLinearColor(0.025f, 0.075f, 0.10f, 0.99f));
    if (UCanvasPanelSlot* PassSlot = Canvas->AddChildToCanvas(Pass))
    {
        PassSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
        PassSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        PassSlot->SetSize(bPaperCopy ? FVector2D(900.0f, 520.0f) : FVector2D(520.0f, 800.0f));
    }

    UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Pass->AddChild(Layout);
    const FLinearColor Ink = bPaperCopy ? FLinearColor(0.035f, 0.07f, 0.09f, 1.0f) : FLinearColor::White;
    const FLinearColor Accent = bPaperCopy ? FLinearColor(0.02f, 0.40f, 0.42f, 1.0f)
                                           : FLinearColor(0.26f, 0.88f, 0.82f, 1.0f);

    auto AddText = [this, Layout, Ink, Accent](const FString& Text, int32 FontSize, bool bBold, bool bAccent)
    {
        UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Block->SetText(FText::FromString(Text));
        Block->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", FontSize));
        Block->SetColorAndOpacity(FSlateColor(bAccent ? Accent : Ink));
        Block->SetAutoWrapText(true);
        if (UVerticalBoxSlot* Slot = Layout->AddChildToVerticalBox(Block))
        {
            Slot->SetPadding(FMargin(0.0f, 3.0f));
        }
    };

    const FString PassTitle = bPaperCopy ? TEXT("COASTLINE AIRWAYS  |  PAPER BOARDING PASS")
                                         : TEXT("COASTLINE AIRWAYS  |  MOBILE BOARDING PASS");
    AddText(PassTitle, 16, true, true);
    AddText(Ticket.PassengerName, 27, true, false);
    AddText(FString::Printf(TEXT("%s    %s    %s"), *Ticket.FlightNumber, *Ticket.Airline,
        Ticket.Status == EGCTicketStatus::CheckedIn || Ticket.Status == EGCTicketStatus::Boarded ? TEXT("CHECKED IN") : TEXT("BOOKED")), 15, true, false);
    AddText(FString::Printf(TEXT("PASSENGER ID  %s"), *Ticket.PassengerID), 11, false, false);
    AddText(FString::Printf(TEXT("FROM  %s  %s"), *Ticket.FromCity, *Ticket.FromCode), 16, true, false);
    AddText(FString::Printf(TEXT("TO      %s  %s"), *Ticket.ToCity, *Ticket.ToCode), 16, true, false);
    AddText(FString::Printf(TEXT("DATE  %s     DEPART  %s     ARRIVE  %s     BOARD  %s"),
        *Ticket.FlightDateUtc.ToString(TEXT("%a, %d %b %Y")), *FString::Printf(TEXT("%02d:%02d"),
        FMath::FloorToInt(Ticket.DepartureHour), FMath::RoundToInt(FMath::Frac(Ticket.DepartureHour) * 60.0f)),
        *Ticket.ArrivalTime, *Ticket.BoardingTime), 13, false, false);
    AddText(FString::Printf(TEXT("TERMINAL  %s      GATE  %s      GROUP  %d"),
        *Ticket.Terminal, *Ticket.Gate, Ticket.BoardingGroup), 14, true, false);
    const TCHAR* CabinName = Ticket.SeatClass == EGCSeatClass::First ? TEXT("First")
        : Ticket.SeatClass == EGCSeatClass::Business ? TEXT("Business") : TEXT("Economy");
    AddText(FString::Printf(TEXT("SEAT  %s      CABIN  %s"), Ticket.Seat.IsEmpty() ? TEXT("CHECK IN REQUIRED") : *Ticket.Seat,
        CabinName), 22, true, true);
    AddText(FString::Printf(TEXT("BOOKING REF  %s      FARE  $%d"), *Ticket.BookingReference, Ticket.PricePaid), 13, false, false);
    AddText(FString::Printf(TEXT("TICKET  %s"), *Ticket.TicketID), 12, false, false);

    UHorizontalBox* Barcode = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    for (int32 Index = 0; Index < 72; ++Index)
    {
        const uint32 Bit = GetTypeHash(FString::Printf(TEXT("%s-%d"), *Ticket.BarcodePayload, Index));
        if ((Bit & 3u) == 0u)
        {
            USizeBox* Gap = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
            Gap->SetWidthOverride(2.0f);
            Barcode->AddChildToHorizontalBox(Gap);
            continue;
        }
        UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Bar->SetBrushColor(Ink);
        USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        BarSize->SetWidthOverride((Bit & 4u) ? 3.0f : 1.5f);
        BarSize->SetHeightOverride(48.0f);
        BarSize->AddChild(Bar);
        Barcode->AddChildToHorizontalBox(BarSize);
    }
    if (UVerticalBoxSlot* BarcodeSlot = Layout->AddChildToVerticalBox(Barcode))
    {
        BarcodeSlot->SetPadding(FMargin(0.0f, 8.0f));
        BarcodeSlot->SetHorizontalAlignment(HAlign_Center);
    }
    AddText(Ticket.BarcodePayload, 9, false, false);
    AddText(bPaperCopy ? TEXT("KEEP THIS PASS UNTIL YOU LEAVE THE ARRIVAL AIRPORT")
                       : TEXT("F7 prints a paper copy  |  F8 returns to your phone"), 10, true, true);
    bBuilt = true;
}
