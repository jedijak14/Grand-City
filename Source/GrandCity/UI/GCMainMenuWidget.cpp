#include "UI/GCMainMenuWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"

namespace
{
	UButton* MakeMenuButton(UWidgetTree* Tree, UVerticalBox* Parent, const TCHAR* Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Label));
		Text->SetJustification(ETextJustify::Center);
		Button->AddChild(Text);
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Button))
		{
			Slot->SetPadding(FMargin(8.0f));
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
	USizeBox* Panel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Panel->SetWidthOverride(520.0f);
	Panel->SetHeightOverride(340.0f);
	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->AddChild(Stack);
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	}

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("GRAND CITY")));
	Title->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TitleSlot = Stack->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(12.0f, 24.0f));
	}
	PlayButton = MakeMenuButton(WidgetTree, Stack, TEXT("PLAY"));
	SettingsButton = MakeMenuButton(WidgetTree, Stack, TEXT("SETTINGS"));
	if (PlayButton)
	{
		PlayButton->OnClicked.AddDynamic(this, &UGCMainMenuWidget::HandlePlayClicked);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddDynamic(this, &UGCMainMenuWidget::HandleSettingsClicked);
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
