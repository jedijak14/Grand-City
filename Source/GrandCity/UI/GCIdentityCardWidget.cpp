#include "UI/GCIdentityCardWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/GCPlayerState.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"

UTexture2D* UGCIdentityCardWidget::GeneratePortrait(const FString& SeedName)
{
    constexpr int32 Width = 128;
    constexpr int32 Height = 128;
    TArray<FColor> Pixels;
    Pixels.SetNumUninitialized(Width * Height);
    const uint32 Seed = GetTypeHash(SeedName);
    const FColor BackgroundA(24 + (Seed & 15), 72 + ((Seed >> 4) & 31), 92 + ((Seed >> 9) & 31), 255);
    const FColor BackgroundB(16, 34, 50, 255);
    const FColor Skin(212, 159, 128, 255);
    const FColor Hair(39, 31, 29, 255);
    const FColor Shirt(35, 100, 107, 255);

    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            const float T = static_cast<float>(Y) / static_cast<float>(Height - 1);
            FColor Color = FColor(
                FMath::RoundToInt(FMath::Lerp(static_cast<float>(BackgroundA.R), static_cast<float>(BackgroundB.R), T)),
                FMath::RoundToInt(FMath::Lerp(static_cast<float>(BackgroundA.G), static_cast<float>(BackgroundB.G), T)),
                FMath::RoundToInt(FMath::Lerp(static_cast<float>(BackgroundA.B), static_cast<float>(BackgroundB.B), T)), 255);
            const float Head = FMath::Square((X - 64.0f) / 23.0f) + FMath::Square((Y - 56.0f) / 31.0f);
            const float HairShape = FMath::Square((X - 64.0f) / 25.0f) + FMath::Square((Y - 39.0f) / 15.0f);
            const float Body = FMath::Square((X - 64.0f) / 43.0f) + FMath::Square((Y - 111.0f) / 32.0f);
            if (Body <= 1.0f)
            {
                Color = Shirt;
            }
            if (Head <= 1.0f)
            {
                Color = Skin;
            }
            if (HairShape <= 1.0f && Y < 50)
            {
                Color = Hair;
            }
            if ((X >= 53 && X <= 57 && Y >= 57 && Y <= 61)
                || (X >= 71 && X <= 75 && Y >= 57 && Y <= 61))
            {
                Color = FColor(35, 32, 31, 255);
            }
            if (Y >= 78 && Y <= 80 && X >= 59 && X <= 69)
            {
                Color = FColor(130, 75, 69, 255);
            }
            Pixels[Y * Width + X] = Color;
        }
    }

    UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
    if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.IsEmpty())
    {
        return nullptr;
    }
    Texture->SRGB = true;
    FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
    void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
    Mip.BulkData.Unlock();
    Texture->UpdateResource();
    return Texture;
}

void UGCIdentityCardWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!WidgetTree)
    {
        return;
    }
    APlayerController* PC = GetOwningPlayer();
    AGCPlayerState* PS = PC ? PC->GetPlayerState<AGCPlayerState>() : nullptr;
    if (!PS || PS->CitizenID.IsEmpty())
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    WidgetTree->RootWidget = Canvas;
    UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Card->SetBrushColor(FLinearColor(0.035f, 0.075f, 0.092f, 0.98f));
    Card->SetPadding(FMargin(26.0f));
    if (UCanvasPanelSlot* CardSlot = Canvas->AddChildToCanvas(Card))
    {
        CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
        CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        CardSlot->SetSize(FVector2D(760.0f, 350.0f));
    }
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Card->AddChild(Row);

    PortraitTexture = GeneratePortrait(PS->CitizenName);
    USizeBox* PortraitSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    PortraitSize->SetWidthOverride(210.0f);
    PortraitSize->SetHeightOverride(238.0f);
    PortraitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    if (PortraitTexture)
    {
        PortraitImage->SetBrushFromTexture(PortraitTexture, true);
    }
    PortraitSize->AddChild(PortraitImage);
    if (UHorizontalBoxSlot* PortraitSlot = Row->AddChildToHorizontalBox(PortraitSize))
    {
        PortraitSlot->SetPadding(FMargin(0.0f, 0.0f, 22.0f, 0.0f));
    }

    UVerticalBox* Details = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Row->AddChildToHorizontalBox(Details);
    auto AddText = [this, Details](const FString& Text, int32 FontSize, const FLinearColor& Color, bool bBold)
    {
        UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Block->SetText(FText::FromString(Text));
        Block->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", FontSize));
        Block->SetColorAndOpacity(FSlateColor(Color));
        if (UVerticalBoxSlot* Slot = Details->AddChildToVerticalBox(Block))
        {
            Slot->SetPadding(FMargin(0.0f, 4.0f));
        }
    };

    AddText(TEXT("GRIDLOCK  /  CITIZEN ID"), 13, FLinearColor(0.26f, 0.85f, 0.76f, 1.0f), true);
    AddText(PS->CitizenName, 27, FLinearColor::White, true);
    AddText(FString::Printf(TEXT("ID  %s"), *PS->CitizenID), 16, FLinearColor(0.78f, 0.87f, 0.9f, 1.0f), false);
    AddText(FString::Printf(TEXT("DOB  %s"), *PS->CitizenDateOfBirth.ToString(TEXT("%Y-%m-%d"))), 15, FLinearColor::White, false);
    AddText(FString::Printf(TEXT("SEX  %s"), *UEnum::GetValueAsString(PS->CitizenSex).RightChop(15)), 15, FLinearColor::White, false);
    AddText(FString::Printf(TEXT("DRIVER LICENSE  %s"), *PS->DriverLicenseType), 15, FLinearColor::White, false);
    AddText(FString::Printf(TEXT("LICENSE NO.  %s"), *PS->DriverLicenseNumber), 13, FLinearColor(0.78f, 0.87f, 0.9f, 1.0f), false);
    AddText(FString::Printf(TEXT("VALID THRU  %s"), *PS->LicenseExpiryUtc.ToString(TEXT("%Y-%m-%d"))), 13, FLinearColor(0.78f, 0.87f, 0.9f, 1.0f), false);
    AddText(FString::Printf(TEXT("RESIDENCE  %s"), PS->CurrentAddress.IsEmpty() ? TEXT("UNASSIGNED") : *PS->CurrentAddress),
        13, FLinearColor(0.78f, 0.87f, 0.9f, 1.0f), false);
    AddText(TEXT("GRIDLOCK INTERNATIONAL  /  VERIFIED PROFILE"), 11, FLinearColor(0.5f, 0.67f, 0.72f, 1.0f), false);
}
