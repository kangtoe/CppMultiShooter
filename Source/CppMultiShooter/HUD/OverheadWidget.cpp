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

	// get netRole
	ENetRole netRole = InPawn->GetLocalRole(); //GetRemoteRole
	FString Role;
	switch (netRole)
	{
	case ENetRole::ROLE_Authority:
		Role = FString("Authority");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role = FString("Autonomous Proxy");
		break;
	case ENetRole::ROLE_SimulatedProxy:
		Role = FString("Simulated Proxy");
		break;
	case ENetRole::ROLE_None:
		Role = FString("None");
		break;
	}
	 displayStr += FString::Printf(TEXT("Remote Role: %s"), *Role);		

	 // get player name
	APlayerState* PlayerState = InPawn->GetPlayerState();	
	if (PlayerState)
	{
		displayStr = PlayerState->GetPlayerName() + TEXT(" | ") + displayStr;
	}

	SetDisplayText(displayStr);
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
