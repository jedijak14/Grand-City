#include "UI/GCIdentitySetupWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/GCPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"

void UGCIdentitySetupWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree)
	{
		return;
	}
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Backdrop->SetBrushColor(FLinearColor(0.018f, 0.034f, 0.052f, 0.98f));
	if (UCanvasPanelSlot* BackdropSlot = Canvas->AddChildToCanvas(Backdrop))
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetOffsets(FMargin(0.0f));
	}

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Card->SetBrushColor(FLinearColor(0.035f, 0.075f, 0.092f, 0.98f));
	Card->SetPadding(FMargin(36.0f));
	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Card->AddChild(Stack);
	if (UCanvasPanelSlot* CardSlot = Canvas->AddChildToCanvas(Card))
	{
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetSize(FVector2D(540.0f, 390.0f));
	}

	auto AddLabel = [this, Stack](const TCHAR* Text, int32 Size, FLinearColor Color, bool bBold)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(Text));
		Label->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
		Label->SetColorAndOpacity(FSlateColor(Color));
		if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(Label))
		{
			Slot->SetPadding(FMargin(0.0f, 5.0f));
		}
		return Label;
	};
	AddLabel(TEXT("GRIDLOCK  /  FIRST-RUN PROFILE"), 13, FLinearColor(0.25f, 0.86f, 0.76f, 1.0f), true);
	AddLabel(TEXT("CREATE YOUR CITY ID"), 32, FLinearColor::White, true);
	AddLabel(TEXT("Choose the name shown on your profile and digital ID."), 15, FLinearColor(0.75f, 0.82f, 0.85f, 1.0f), false);

	NameField = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	NameField->SetHintText(FText::FromString(TEXT("Display name (2–24 characters)")));
	if (UVerticalBoxSlot* NameSlot = Stack->AddChildToVerticalBox(NameField))
	{
		NameSlot->SetPadding(FMargin(0.0f, 20.0f, 0.0f, 10.0f));
	}
	FeedbackText = AddLabel(TEXT("A unique Gridlock citizen ID is generated when you continue."), 12,
		FLinearColor(0.66f, 0.76f, 0.80f, 1.0f), false);

	UButton* CreateButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CreateButton->SetBackgroundColor(FLinearColor(0.12f, 0.61f, 0.54f, 1.0f));
	UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ButtonLabel->SetText(FText::FromString(TEXT("CREATE ID  /  CONTINUE")));
	ButtonLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 15));
	ButtonLabel->SetJustification(ETextJustify::Center);
	ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CreateButton->AddChild(ButtonLabel);
	if (UVerticalBoxSlot* ButtonSlot = Stack->AddChildToVerticalBox(CreateButton))
	{
		ButtonSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));
	}
	CreateButton->OnClicked.AddDynamic(this, &UGCIdentitySetupWidget::HandleCreateIdentity);
}

void UGCIdentitySetupWidget::HandleCreateIdentity()
{
	APlayerController* PC = GetOwningPlayer();
	AGCPlayerState* PlayerState = PC ? PC->GetPlayerState<AGCPlayerState>() : nullptr;
	const FString DisplayName = NameField ? NameField->GetText().ToString() : FString();
	if (!PlayerState || !PlayerState->CreateCitizenIdentity(DisplayName))
	{
		if (FeedbackText)
		{
			FeedbackText->SetText(FText::FromString(TEXT("Please enter a name with at least two characters.")));
			FeedbackText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.35f, 0.30f, 1.0f)));
		}
		return;
	}
	if (FeedbackText)
	{
		FeedbackText->SetText(FText::FromString(FString::Printf(TEXT("ID CREATED: %s"), *PlayerState->CitizenID)));
	}
	OnIdentitySetupComplete.Broadcast();
}
