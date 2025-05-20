// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "CppMultiShooter/HUD/LobbyWait.h"

#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class CPPMULTISHOOTER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<class ULobbyWait> LobbyWaitWidgetClass;
	
protected:    
    class ULobbyWait* LobbyWait;

    void StartCountdown();
    void UpdateCountdown();
    void OnCountdownFinished();

    int32 CountdownValue = 3;
    FTimerHandle CountdownTimerHandle;
};
