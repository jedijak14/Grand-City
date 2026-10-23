#include "UI/GCPoliceActionWheelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Styling/CoreStyle.h"

void UGCPoliceActionWheelWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RebuildWheel();
}

void UGCPoliceActionWheelWidget::ConfigureForVehicle(bool bInVehicleMode)
{
    if (bVehicleMode != bInVehicleMode)
    {
        bVehicleMode = bInVehicleMode;
        if (bBuilt)
        {
            RebuildWheel();
        }
    }
}

void UGCPoliceActionWheelWidget::RebuildWheel()
{
    if (!WidgetTree)
    {
        return;
    }
    WidgetTree->RootWidget = nullptr;
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    WidgetTree->RootWidget = Canvas;
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Backdrop->SetBrushColor(FLinearColor(0.01f, 0.025f, 0.035f, 0.72f));
    if (UCanvasPanelSlot* BackdropSlot = Canvas->AddChildToCanvas(Backdrop))
    {
        BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        BackdropSlot->SetOffsets(FMargin(0.0f));
    }

    UTextBlock* Header = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Header->SetText(FText::FromString(bVehicleMode ? TEXT("TRAFFIC ACTIONS") : TEXT("CITIZEN ACTIONS")));
    Header->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 20));
    Header->SetColorAndOpacity(FSlateColor(FLinearColor(0.32f, 0.91f, 0.82f, 1.0f)));
    if (UCanvasPanelSlot* HeaderSlot = Canvas->AddChildToCanvas(Header))
    {
        HeaderSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
        HeaderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        HeaderSlot->SetPosition(FVector2D(0.0f, -26.0f));
        HeaderSlot->SetSize(FVector2D(360.0f, 42.0f));
    }

    TArray<TPair<FName, FString>> Actions;
    if (bVehicleMode)
    {
        Actions = {
            { TEXT("INSPECT"), TEXT("Inspect plate / record") }, { TEXT("TRAFFIC_STOP"), TEXT("Initiate traffic stop") },
            { TEXT("BREATH_TEST"), TEXT("Breathalyzer") }, { TEXT("SPEEDING"), TEXT("Cite speeding") },
            { TEXT("TAIL_LIGHT"), TEXT("Cite broken light") }, { TEXT("EXPIRED_METER"), TEXT("Cite expired meter") },
            { TEXT("HYDRANT"), TEXT("Cite hydrant block") }, { TEXT("WRONG_WAY"), TEXT("Cite wrong-way park") },
            { TEXT("SMOKE"), TEXT("Cite smoke / exhaust") }, { TEXT("DRUG_TEST"), TEXT("Drug impairment test") }
        };
    }
    else
    {
        Actions = {
            { TEXT("ID"), TEXT("Request / verify ID") }, { TEXT("INTERVIEW"), TEXT("Interview witness") },
            { TEXT("FRISK"), TEXT("Frisk (reasonable suspicion)") }, { TEXT("SEARCH"), TEXT("Search person / bag") },
            { TEXT("ARREST"), TEXT("Arrest (legal basis required)") }, { TEXT("RELEASE"), TEXT("Release from stop") },
            { TEXT("JAYWALKING"), TEXT("Cite jaywalking") }, { TEXT("LITTERING"), TEXT("Cite littering") },
            { TEXT("LOITERING"), TEXT("Cite loitering") }, { TEXT("ALCOHOL_TEST"), TEXT("Alcohol test") },
            { TEXT("DRUG_TEST"), TEXT("Drug test") }, { TEXT("FAKE_ID"), TEXT("Cite fake ID") },
            { TEXT("EXPIRED_LICENSE"), TEXT("Cite expired license") }
        };
    }

    const float Radius = Actions.Num() > 9 ? 292.0f : 245.0f;
    const float ActionButtonWidth = Actions.Num() > 9 ? 142.0f : 220.0f;
    for (int32 Index = 0; Index < Actions.Num(); ++Index)
    {
        const float Angle = (-90.0f + 360.0f * Index / Actions.Num()) * PI / 180.0f;
        const FVector2D Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
        Button->SetBackgroundColor(FLinearColor(0.035f, 0.13f, 0.15f, 0.96f));
        UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        UBorder* IconPlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        IconPlate->SetBrushColor(FLinearColor(0.08f, 0.34f, 0.35f, 1.0f));
        UTextBlock* Icon = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        const FName ActionKey = Actions[Index].Key;
        FString IconToken = TEXT("INFO");
        if (ActionKey == TEXT("INSPECT")) IconToken = TEXT("PLT");
        else if (ActionKey == TEXT("TRAFFIC_STOP")) IconToken = TEXT("STOP");
        else if (ActionKey == TEXT("BREATH_TEST") || ActionKey == TEXT("ALCOHOL_TEST")) IconToken = TEXT("BAC");
        else if (ActionKey == TEXT("DRUG_TEST")) IconToken = TEXT("DRUG");
        else if (ActionKey == TEXT("SPEEDING")) IconToken = TEXT("SPD");
        else if (ActionKey == TEXT("TAIL_LIGHT")) IconToken = TEXT("LAMP");
        else if (ActionKey == TEXT("EXPIRED_METER")) IconToken = TEXT("MTR");
        else if (ActionKey == TEXT("HYDRANT")) IconToken = TEXT("HYD");
        else if (ActionKey == TEXT("WRONG_WAY")) IconToken = TEXT("WAY");
        else if (ActionKey == TEXT("SMOKE")) IconToken = TEXT("EXH");
        else if (ActionKey == TEXT("ID")) IconToken = TEXT("ID");
        else if (ActionKey == TEXT("INTERVIEW")) IconToken = TEXT("WIT");
        else if (ActionKey == TEXT("FRISK")) IconToken = TEXT("PAT");
        else if (ActionKey == TEXT("SEARCH")) IconToken = TEXT("SRCH");
        else if (ActionKey == TEXT("ARREST")) IconToken = TEXT("AR");
        else if (ActionKey == TEXT("RELEASE")) IconToken = TEXT("REL");
        else if (ActionKey == TEXT("JAYWALKING")) IconToken = TEXT("JAY");
        else if (ActionKey == TEXT("LITTERING")) IconToken = TEXT("LIT");
        else if (ActionKey == TEXT("LOITERING")) IconToken = TEXT("LOIT");
        else if (ActionKey == TEXT("FAKE_ID")) IconToken = TEXT("FAKE");
        else if (ActionKey == TEXT("EXPIRED_LICENSE")) IconToken = TEXT("EXP");
        Icon->SetText(FText::FromString(IconToken));
        Icon->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 9));
        Icon->SetJustification(ETextJustify::Center);
        Icon->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 1.0f, 0.96f, 1.0f)));
        IconPlate->SetPadding(FMargin(5.0f, 4.0f));
        IconPlate->SetContent(Icon);
        Content->AddChildToHorizontalBox(IconPlate);
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Label->SetText(FText::FromString(Actions[Index].Value));
        Label->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
        Label->SetJustification(ETextJustify::Left);
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        if (UHorizontalBoxSlot* LabelSlot = Content->AddChildToHorizontalBox(Label))
        {
            LabelSlot->SetPadding(FMargin(9.0f, 0.0f, 0.0f, 0.0f));
            LabelSlot->SetVerticalAlignment(VAlign_Center);
        }
        Button->AddChild(Content);
        if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Button))
        {
            Slot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
            Slot->SetAlignment(FVector2D(0.5f, 0.5f));
            Slot->SetPosition(Offset);
            Slot->SetSize(FVector2D(ActionButtonWidth, 54.0f));
        }
        const FName ActionID = Actions[Index].Key;
        Button->OnClicked.AddLambda([this, ActionID]()
        {
            OnActionSelected.Broadcast(ActionID);
        });
    }

    UTextBlock* Footer = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Footer->SetText(FText::FromString(TEXT("Hold E to use actions  •  Release E to close  •  Unsupported actions cost conduct points")));
    Footer->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
    Footer->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.81f, 0.84f, 1.0f)));
    if (UCanvasPanelSlot* FooterSlot = Canvas->AddChildToCanvas(Footer))
    {
        FooterSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
        FooterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        FooterSlot->SetPosition(FVector2D(0.0f, 390.0f));
        FooterSlot->SetSize(FVector2D(760.0f, 32.0f));
    }
    bBuilt = true;
}
