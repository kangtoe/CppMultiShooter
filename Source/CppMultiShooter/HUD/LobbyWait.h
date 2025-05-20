// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWait.generated.h"

/**
 * 
 */
UCLASS()
class CPPMULTISHOOTER_API ULobbyWait : public UUserWidget
{
	GENERATED_BODY()
public:        

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* UserCountText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* WaitingText;

    UFUNCTION(Category = "UI")
    void SetUserCountText(const FText& InText);

    UFUNCTION(Category = "UI")
    void SetWaitingText(const FText& InText);
};
