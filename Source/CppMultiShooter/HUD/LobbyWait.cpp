// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyWait.h"

#include "LobbyWait.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void ULobbyWait::SetUserCountText(const FText& InText)
{
    if (UserCountText)
    {
        UserCountText->SetText(InText);
    }
}

void ULobbyWait::SetWaitingText(const FText& InText)
{
    if (WaitingText)
    {
        WaitingText->SetText(InText);
    }
}