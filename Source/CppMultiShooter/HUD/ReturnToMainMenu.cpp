// Fill out your copyright notice in the Description page of Project Settings.


#include "ReturnToMainMenu.h"
#include "GameFramework/PlayerController.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"

void UReturnToMainMenu::MenuSetup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	UWorld* World = GetWorld();
	if (World)
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController)
		{
			FInputModeGameAndUI InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}
	if (ReturnButton && !ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (MultiplayerSessionsSubsystem && !MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
		{
			MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &UReturnToMainMenu::OnDestroySession);
		}
	}
}

bool UReturnToMainMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	return true;
}

void UReturnToMainMenu::OnDestroySession(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("OnDestroySession(): %s"), bWasSuccessful ? TEXT("Success") : TEXT("Fail"));

	if (!bWasSuccessful)
	{
		ReturnButton->SetIsEnabled(true);

		// 이미 세션 없음 → 직접 메인메뉴로
		UWorld* World = GetWorld();
		if (World)
		{
			APlayerController* PC = World->GetFirstPlayerController();
			if (PC)
			{
				UE_LOG(LogTemp, Warning, TEXT("세션 없음: 클라이언트에서 바로 로비로 이동"));
				PC->ClientTravel(TEXT("/Game/Maps/Start"), ETravelType::TRAVEL_Absolute);				
			}
		}

		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>();
		if (GameMode)
		{
			UE_LOG(LogTemp, Warning, TEXT("Calling ReturnToMainMenuHost()"));
			GameMode->ReturnToMainMenuHost();
		}
		else
		{
			PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
			if (PlayerController)
			{
				UE_LOG(LogTemp, Warning, TEXT("Calling ClientReturnToMainMenuWithTextReason()"));
				PlayerController->ClientReturnToMainMenuWithTextReason(FText());
			}
		}
	}
}

void UReturnToMainMenu::MenuTearDown()
{
	UE_LOG(LogTemp, Warning, TEXT("MenuTearDown()"))

	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}

		if (ReturnButton && ReturnButton->OnClicked.IsBound())
		{
			ReturnButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
		}
		if (MultiplayerSessionsSubsystem && MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
		{
			MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(this, &UReturnToMainMenu::OnDestroySession);
		}
	}
}

void UReturnToMainMenu::ReturnButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("ReturnButtonClicked()"))

	ReturnButton->SetIsEnabled(false);
	
	UWorld* World = GetWorld();
	if (World)
	{		
		APlayerController* FirstPlayerController = World->GetFirstPlayerController();
		if (FirstPlayerController)
		{
			AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(FirstPlayerController->GetPawn());
			if (ShooterCharacter)
			{
				ShooterCharacter->ServerLeaveGame();
				ShooterCharacter->OnLeftGame.AddDynamic(this, &UReturnToMainMenu::OnPlayerLeftGame);
			}
			else
			{
				ReturnButton->SetIsEnabled(true);
			}
		}
	}


}

void UReturnToMainMenu::OnPlayerLeftGame()
{
	UE_LOG(LogTemp, Warning, TEXT("OnPlayerLeftGame()"))
	if (MultiplayerSessionsSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("MultiplayerSessionsSubsystem valid"))
		MultiplayerSessionsSubsystem->DestroySession();
	}
}