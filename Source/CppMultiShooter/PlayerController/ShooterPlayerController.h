// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "ShooterPlayerController.generated.h"

UCLASS()
class CPPMULTISHOOTER_API AShooterPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    void SetHUDHealth(float Health, float MaxHealth);
    void SetHUDScore(float Score);
    void SetHUDDefeats(int32 Defeats);

    virtual void OnPossess(APawn* InPawn) override;

protected:
    virtual void BeginPlay() override;    

private:
    class AShooterHUD* ShooterHUD;
};