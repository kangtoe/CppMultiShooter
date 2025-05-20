// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "MultiplayerSessionsSubsystem.h"
#include "CppMultiShooter/HUD/LobbyWait.h"

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	if (!LobbyWait)
	{
		LobbyWait = Cast<ULobbyWait>(CreateWidget<UUserWidget>(GetWorld(), LobbyWaitWidgetClass));
		LobbyWait->AddToViewport();
	}
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!LobbyWait)
	{
		LobbyWait = Cast<ULobbyWait>(CreateWidget<UUserWidget>(GetWorld(), LobbyWaitWidgetClass));
		LobbyWait->AddToViewport();
	}

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
	int32 maxPlayers = 2;
	if (LobbyWait)
	{
		FText FormattedText = FText::Format(
			FText::FromString("{0} / {1}"),
			FText::AsNumber(NumberOfPlayers),
			FText::AsNumber(maxPlayers)
		);

		LobbyWait->SetUserCountText(FormattedText);

		LobbyWait->SetWaitingText(FText::FromString("waiting for others..."));
		if (NumberOfPlayers == maxPlayers)
		{
			LobbyWait->SetWaitingText(FText::FromString("start game soon!"));
			StartCountdown();
			;
		}
	}
}

void ALobbyGameMode::StartCountdown()
{
	CountdownValue = 3;
	UpdateCountdown();

	GetWorld()->GetTimerManager().SetTimer(
		CountdownTimerHandle, this,
		&ALobbyGameMode::UpdateCountdown,
		1.0f, true
	);
}

void ALobbyGameMode::UpdateCountdown()
{
	/*if (LobbyWait)
	{
		FText FormattedText = FText::Format(
			FText::FromString("Start in {0}"),
			FText::AsNumber(CountdownValue)
		);

		LobbyWait->SetWaitingText(FormattedText);
	}*/

	if (CountdownValue <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
		OnCountdownFinished();
	}
	else
	{
		CountdownValue--;
	}
}

void ALobbyGameMode::OnCountdownFinished()
{
	UWorld* World = GetWorld();
	if (World)
	{		
		UGameInstance* GameInstance = GetGameInstance();
		if (GameInstance)
		{
			UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
			check(Subsystem);
			//Subsystem->DesiredNumPublicConnections
			/*FString MatchType = Subsystem->DesiredMatchType;
			if (MatchType == "FreeForAll")
			{
				World->ServerTravel(FString("/Game/Maps/ShooterMap?listen"));
			}
			else if (MatchType == "Teams")
			{
				World->ServerTravel(FString("/Game/Maps/Teams?listen"));
			}
			else if (MatchType == "CaptureTheFlag")
			{
				World->ServerTravel(FString("/Game/Maps/CaptureTheFlag?listen"));
			}*/

			bUseSeamlessTravel = true;
			World->ServerTravel(FString("/Game/Maps/ShooterMap?listen"));
		}
	}	
}