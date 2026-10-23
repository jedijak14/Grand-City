#include "UI/GCMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor GridlockInk(0.012f, 0.025f, 0.045f, 0.92f);
	const FLinearColor GridlockAccent(0.22f, 0.87f, 0.75f, 1.0f);
	const FLinearColor GridlockMuted(0.74f, 0.81f, 0.86f, 1.0f);

	UTextBlock* AddText(UWidgetTree* Tree, UVerticalBox* Parent, const FString& Text,
		int32 Size, const FLinearColor& Color, bool bBold = false)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Block->SetText(FText::FromString(Text));
		Block->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
		Block->SetColorAndOpacity(FSlateColor(Color));
		Parent->AddChildToVerticalBox(Block);
		return Block;
	}

	UButton* AddTile(UWidgetTree* Tree, UCanvasPanel* Canvas, const FString& Eyebrow,
		const FString& Title, const FString& Detail, FVector2D Position, FVector2D Size,
		bool bPrimary, TObjectPtr<UTextBlock>* OutTitle)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		Button->SetBackgroundColor(FLinearColor::Transparent);
		UBorder* Card = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Card->SetBrushColor(bPrimary ? FLinearColor(0.025f, 0.09f, 0.11f, 0.94f) : GridlockInk);
		Card->SetPadding(bPrimary ? FMargin(34.0f) : FMargin(22.0f));
		UVerticalBox* Stack = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Card->AddChild(Stack);
		AddText(Tree, Stack, Eyebrow, 12, GridlockAccent, true);
		UTextBlock* TileTitle = AddText(Tree, Stack, Title, bPrimary ? 40 : 22, FLinearColor::White, true);
		if (OutTitle)
		{
			*OutTitle = TileTitle;
		}
		AddText(Tree, Stack, Detail, bPrimary ? 17 : 13, GridlockMuted);
		Button->AddChild(Card);
		if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Button))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
		}
		return Button;
	}
}

void UGCMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;
	UImage* Backdrop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	TSoftObjectPtr<UTexture2D> Background(FSoftObjectPath(TEXT(
		"/Game/Gridlock/UI/gridlock-loading-coast.gridlock-loading-coast")));
	if (UTexture2D* Texture = Background.LoadSynchronous())
	{
		Backdrop->SetBrushFromTexture(Texture, false);
	}
	else
	{
		Backdrop->SetColorAndOpacity(FLinearColor(0.10f, 0.20f, 0.27f, 1.0f));
	}
	if (UCanvasPanelSlot* BackgroundSlot = Canvas->AddChildToCanvas(Backdrop))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackgroundSlot->SetOffsets(FMargin(0.0f));
	}
	UBorder* Shade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Shade->SetBrushColor(FLinearColor(0.008f, 0.02f, 0.035f, 0.48f));
	if (UCanvasPanelSlot* ShadeSlot = Canvas->AddChildToCanvas(Shade))
	{
		ShadeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		ShadeSlot->SetOffsets(FMargin(0.0f));
	}

	UVerticalBox* BrandStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	AddText(WidgetTree, BrandStack, TEXT("GRIDLOCK"), 34, FLinearColor::White, true);
	AddText(WidgetTree, BrandStack, TEXT("STORY MODE  /  COASTAL CITY"), 11, GridlockAccent, true);
	if (UCanvasPanelSlot* BrandSlot = Canvas->AddChildToCanvas(BrandStack))
	{
		BrandSlot->SetPosition(FVector2D(48.0f, 28.0f));
		BrandSlot->SetSize(FVector2D(420.0f, 90.0f));
	}

	NavLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NavLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 15));
	NavLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	NavLabel->SetJustification(ETextJustify::Right);
	if (UCanvasPanelSlot* NavSlot = Canvas->AddChildToCanvas(NavLabel))
	{
		NavSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
		NavSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		NavSlot->SetPosition(FVector2D(-48.0f, 48.0f));
		NavSlot->SetSize(FVector2D(850.0f, 45.0f));
	}

	PlayButton = AddTile(WidgetTree, Canvas, TEXT("STORY  /  RESUME YOUR JOURNEY"),
		TEXT("CONTINUE"), TEXT("Return to the streets, the coast and the people who make the city."),
		FVector2D(48.0f, 155.0f), FVector2D(700.0f, 480.0f), true, &MainActionLabel);
	ExploreTile = AddTile(WidgetTree, Canvas, TEXT("EXPLORE"), TEXT("CITY & ACTIVITIES"),
		TEXT("A living place to drive, work, travel and make your own choices."),
		FVector2D(780.0f, 155.0f), FVector2D(420.0f, 145.0f), false, nullptr);
	UTextBlock* TileTitle = nullptr;
	AddTile(WidgetTree, Canvas, TEXT("YOUR GARAGE"), TEXT("VEHICLES"),
		TEXT("Cars, aircraft and a growing collection of rides."),
		FVector2D(780.0f, 325.0f), FVector2D(420.0f, 145.0f), false, &TileTitle);
	AddTile(WidgetTree, Canvas, TEXT("MAKE YOUR MARK"), TEXT("CAREERS & HOMES"),
		TEXT("Earn a living, find a place to stay, and shape your story."),
		FVector2D(780.0f, 495.0f), FVector2D(420.0f, 140.0f), false, &TileTitle);

	UButton* SettingsAction = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	SettingsAction->SetBackgroundColor(FLinearColor(0.02f, 0.07f, 0.09f, 0.88f));
	UTextBlock* SettingsLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SettingsLabel->SetText(FText::FromString(TEXT("SETTINGS")));
	SettingsLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
	SettingsLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	SettingsLabel->SetJustification(ETextJustify::Center);
	SettingsAction->AddChild(SettingsLabel);
	if (UCanvasPanelSlot* SettingsSlot = Canvas->AddChildToCanvas(SettingsAction))
	{
		SettingsSlot->SetAnchors(FAnchors(1.0f, 1.0f, 1.0f, 1.0f));
		SettingsSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		SettingsSlot->SetPosition(FVector2D(-48.0f, -35.0f));
		SettingsSlot->SetSize(FVector2D(180.0f, 50.0f));
	}
	SettingsAction->OnClicked.AddDynamic(this, &UGCMainMenuWidget::HandleSettingsClicked);

	if (PlayButton)
	{
		PlayButton->OnClicked.AddDynamic(this, &UGCMainMenuWidget::HandlePlayClicked);
	}
	if (ExploreTile)
	{
		ExploreTile->SetIsEnabled(false);
	}
	SetPauseMode(false);
}

void UGCMainMenuWidget::SetPauseMode(bool bInPauseMode)
{
	bPauseMode = bInPauseMode;
	if (NavLabel)
	{
		NavLabel->SetText(FText::FromString(bPauseMode
			? TEXT("MAP     MISSIONS     INVENTORY     PHONE     STATS     SKILLS     SETTINGS")
			: TEXT("STORY     ONLINE  /  COMING LATER")));
	}
	if (MainActionLabel)
	{
		MainActionLabel->SetText(FText::FromString(bPauseMode ? TEXT("RESUME") : TEXT("CONTINUE")));
	}
}

void UGCMainMenuWidget::HandlePlayClicked()
{
	OnPlayRequested.Broadcast();
}

void UGCMainMenuWidget::HandleSettingsClicked()
{
	OnSettingsRequested.Broadcast();
}
