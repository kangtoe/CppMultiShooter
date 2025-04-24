// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterPlayerController.h"

#include "CppMultiShooter/HUD/ShooterHUD.h"
#include "CppMultiShooter/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "CppMultiShooter/GameMode/ShooterGameMode.h"
#include "CppMultiShooter/PlayerState/ShooterPlayerState.h"
#include "CppMultiShooter/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "CppMultiShooter/ShooterComponents/CombatComponent.h"
#include "CppMultiShooter/Weapon/Weapon.h"
#include "CppMultiShooter/GameState/ShooterGameState.h"
#include "CppMultiShooter/HUD/ScopeWidget.h"

void AShooterPlayerController::BeginPlay()
{
    Super::BeginPlay();

    ShooterHUD = Cast<AShooterHUD>(GetHUD());
    
    ServerCheckMatchState();
}

void AShooterPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(InPawn);
    if (ShooterCharacter)
    {
        SetHUDHealth(ShooterCharacter->GetHealth(), ShooterCharacter->GetMaxHealth()); // update health HUD
    }
}

void AShooterPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    SetHUDTime();
    CheckTimeSync(DeltaTime);
}

void AShooterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AShooterPlayerController, MatchState);
}

void AShooterPlayerController::SetHUDScore(float Score)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    bool bHUDValid = ShooterHUD &&
        ShooterHUD->CharacterOverlay &&
        ShooterHUD->CharacterOverlay->ScoreAmount;
    if (bHUDValid)
    {
        FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
        ShooterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
    }
    else
    {
        bInitializeCharacterOverlay = true;
        HUDScore = Score;
    }
}

void AShooterPlayerController::SetHUDDefeats(int32 Defeats)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    bool bHUDValid = ShooterHUD &&
        ShooterHUD->CharacterOverlay &&
        ShooterHUD->CharacterOverlay->ScoreAmount;
    if (bHUDValid)
    {
        FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
        ShooterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
    }
    else
    {
        bInitializeCharacterOverlay = true;
        HUDDefeats = Defeats;
    }
}

void AShooterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;

    bool bHUDValid = ShooterHUD &&
        ShooterHUD->CharacterOverlay &&
        ShooterHUD->CharacterOverlay->WeaponAmmoAmount;
    if (bHUDValid)
    {
        FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
        ShooterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
    }
}

void AShooterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    bool bHUDValid = ShooterHUD &&
        ShooterHUD->CharacterOverlay &&
        ShooterHUD->CharacterOverlay->CarriedAmmoAmount;
    if (bHUDValid)
    {
        FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
        ShooterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
    }
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
    else
    {
        bInitializeCharacterOverlay = true;
        HUDHealth = Health;
        HUDMaxHealth = MaxHealth;
    }
}

void AShooterPlayerController::SetHUDTime()
{
    // for server (may ServerCheckMatchState() is called in the PlayerController's BeginPlay())
    if (HasAuthority())
    {
        AShooterGameMode* ShooterGameMode = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this));
        if (ShooterGameMode == nullptr)
        {
            ShooterGameMode = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this));
            LevelStartingTime = ShooterGameMode->LevelStartingTime;
        }        
        if (ShooterGameMode)
        {
            LevelStartingTime = ShooterGameMode->LevelStartingTime;
            //or -> LevelStartingTime = BlasterGameMode->GetLevelStartingTime();
        }
    }

    float TimeLeft = 0.f;
    if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
    else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
    else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;

    uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
    if (TimeLeft < 0) TimeLeft = 0;

    if (CountdownInt != SecondsLeft)
    {
        if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
        {
            SetHUDAnnouncementCountdown(TimeLeft);
        }
        if (MatchState == MatchState::InProgress)
        {
            SetHUDMatchCountdown(TimeLeft);
        }
    }

    CountdownInt = SecondsLeft;
}

void AShooterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    bool bHUDValid = ShooterHUD &&
        ShooterHUD->Announcement &&
        ShooterHUD->Announcement->WarmupTime;

    if (bHUDValid)
    {
        int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
        int32 Seconds = CountdownTime - Minutes * 60;

        FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
        ShooterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
    }
}

void AShooterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    bool bHUDValid = ShooterHUD &&
        ShooterHUD->CharacterOverlay &&
        ShooterHUD->CharacterOverlay->MatchCountdownText;

    if (bHUDValid)
    {
        if (CountdownTime < 0.f)
        {
            ShooterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
            return;
        }


        int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
        int32 Seconds = CountdownTime - Minutes * 60;

        FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
        ShooterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
    }
}

void AShooterPlayerController::PollInit()
{
    if (CharacterOverlay == nullptr)
    {
        if (ShooterHUD && ShooterHUD->CharacterOverlay)
        {
            CharacterOverlay = ShooterHUD->CharacterOverlay;
            if (CharacterOverlay)
            {
                SetHUDHealth(HUDHealth, HUDMaxHealth);
                SetHUDScore(HUDScore);
                SetHUDDefeats(HUDDefeats);
            }
        }
    }
}

void AShooterPlayerController::CheckTimeSync(float DeltaTime)
{
    TimeSyncRunningTime += DeltaTime;
    if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
    {
        ServerRequestServerTime(GetWorld()->GetTimeSeconds());
        TimeSyncRunningTime = 0.f;
    }
}

void AShooterPlayerController::ServerCheckMatchState_Implementation() // 웜업 시간 중 클라이언트 조인
{
    AShooterGameMode* GameMode = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this));
    if (GameMode)
    {
        WarmupTime = GameMode->WarmupTime;
        MatchTime = GameMode->MatchTime;
        CooldownTime = GameMode->CooldownTime;
        LevelStartingTime = GameMode->LevelStartingTime;
        MatchState = GameMode->GetMatchState();
        ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
    }
}

void AShooterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
    WarmupTime = Warmup;
    MatchTime = Match;
    CooldownTime = Cooldown;
    LevelStartingTime = StartingTime;
    MatchState = StateOfMatch;
    OnMatchStateSet(MatchState);
    if (ShooterHUD && MatchState == MatchState::WaitingToStart)
    {
        ShooterHUD->AddAnnouncement();
    }
}

void AShooterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest) 
{
    float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds(); 
    ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AShooterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
    float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
    float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
    ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float AShooterPlayerController::GetServerTime()
{
    if (HasAuthority()) return GetWorld()->GetTimeSeconds();
    else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void AShooterPlayerController::ReceivedPlayer()
{
    Super::ReceivedPlayer();
    if (IsLocalController())
    {
        ServerRequestServerTime(GetWorld()->GetTimeSeconds());
    }
}

void AShooterPlayerController::OnMatchStateSet(FName State)
{
    MatchState = State;

    if (MatchState == MatchState::InProgress)
    {
        HandleMatchHasStarted();
    }
    else if (MatchState == MatchState::Cooldown)
    {
        HandleCooldown();
    }
}

void AShooterPlayerController::OnRep_MatchState()
{
    if (MatchState == MatchState::InProgress)
    {
        HandleMatchHasStarted();
    }
    else if (MatchState == MatchState::Cooldown)
    {
        HandleCooldown();
    }
}

void AShooterPlayerController::HandleMatchHasStarted()
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    if (ShooterHUD)
    {
        ShooterHUD->AddCharacterOverlay();
        if (ShooterHUD->Announcement)
        {            
            ShooterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void AShooterPlayerController::HandleCooldown()
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    if (ShooterHUD)
    {
        ShooterHUD->CharacterOverlay->RemoveFromParent();

        bool bHUDValid = ShooterHUD->Announcement &&
            ShooterHUD->Announcement->AnnouncementText &&
            ShooterHUD->Announcement->InfoText;

        if (bHUDValid)
        {
            ShooterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
            FString AnnouncementText("New Match Starts In:");
            ShooterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
            
            // set text top players
            AShooterGameState* ShooterGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
            AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>();
            if (ShooterGameState && ShooterPlayerState)
            {
                TArray<AShooterPlayerState*> TopPlayers = ShooterGameState->TopScoringPlayers;
                FString InfoTextString;
                if (TopPlayers.Num() == 0)
                {
                    InfoTextString = FString("There is no winner.");
                }
                else if (TopPlayers.Num() == 1 && TopPlayers[0] == ShooterPlayerState)
                {
                    InfoTextString = FString("You are the winner!");
                }
                else if (TopPlayers.Num() == 1)
                {
                    InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
                }
                else if (TopPlayers.Num() > 1)
                {
                    InfoTextString = FString("Players tied for the win:\n");
                    for (auto TiedPlayer : TopPlayers)
                    {
                        InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
                    }
                }
                ShooterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
            }
        }

        // disable gameplay
        AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(GetPawn());
        if (ShooterCharacter && ShooterCharacter->GetCombat())
        {
            ShooterCharacter->bDisableGameplay = true;
            ShooterCharacter->GetCombat()->SetFiring(false);
        }
    }
}

void AShooterPlayerController::SetHUDScope(bool bIsAiming)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;

    UE_LOG(LogTemp, Warning, TEXT("bIsAiming: %d"), bIsAiming);
    bool bHUDValid = ShooterHUD &&
        ShooterHUD->ScopeWidget &&
        ShooterHUD->ScopeWidget->ScopeZoomIn;
    UE_LOG(LogTemp, Warning, TEXT("bIsAiming: %d"), bIsAiming);

    if (!ShooterHUD->ScopeWidget)
    {
        ShooterHUD->AddScopeWidget();
    }

    if (bHUDValid)
    {
        if (bIsAiming)
        {
            ShooterHUD->ScopeWidget->PlayAnimation(ShooterHUD->ScopeWidget->ScopeZoomIn);
        }
        else
        {
            ShooterHUD->ScopeWidget->PlayAnimation(
                ShooterHUD->ScopeWidget->ScopeZoomIn,
                0.f,
                1,
                EUMGSequencePlayMode::Reverse
            );
        }
    }
}
