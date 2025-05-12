// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterOverlay.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UCharacterOverlay::SetPingUIColor(FColor color)
{
    PingAmountText->SetColorAndOpacity(FSlateColor(color));
    PingImage->SetColorAndOpacity(FLinearColor(color));
}
