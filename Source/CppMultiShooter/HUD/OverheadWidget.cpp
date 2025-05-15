// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	FString displayStr;
	
	FString Role = UEnum::GetValueAsString(InPawn->GetLocalRole());
	//displayStr += TEXT(" | ") + FString::Printf(TEXT("Remote Role: %s"), *Role);		

	 // get player name
	APlayerState* PlayerState = InPawn->GetPlayerState();	
	if (PlayerState && InPawn->IsLocallyControlled())
	{
		displayStr = PlayerState->GetPlayerName();
	}

	SetDisplayText(displayStr);
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
