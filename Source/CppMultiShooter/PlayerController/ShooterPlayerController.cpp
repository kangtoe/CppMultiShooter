// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterPlayerController.h"

#include "CppMultiShooter/HUD/ShooterHUD.h"
#include "CppMultiShooter/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void AShooterPlayerController::BeginPlay()
{
    Super::BeginPlay();

    ShooterHUD = Cast<AShooterHUD>(GetHUD());
}

void AShooterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;

    bool bHUDValid = ShooterHUD &&
        ShooterHUD->CharacterOverlay &&
        ShooterHUD->CharacterOverlay->HealthBar &&
        ShooterHUD->CharacterOverlay->HealthText;
    if (bHUDValid)
    {
        const float HealthPercent = Health / MaxHealth;
        ShooterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
        FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
        ShooterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
    }
}