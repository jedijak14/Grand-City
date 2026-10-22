#include "UI/GCLoadingScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UGCLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;
	USizeBox* Panel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Panel->SetWidthOverride(700.0f);
	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->AddChild(Stack);
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.78f, 0.5f, 0.78f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	}

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("GRAND CITY  |  PREPARING THE CITY")));
	Title->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TitleSlot = Stack->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(8.0f));
	}
	ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	Stack->AddChildToVerticalBox(ProgressBar);
	SetProgress(0.0f);
}

void UGCLoadingScreenWidget::SetProgress(float Progress)
{
	if (ProgressBar)
	{
		ProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
	}
}
