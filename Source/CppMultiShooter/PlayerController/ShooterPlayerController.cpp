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
#include "CppMultiShooter/HUD/ReturnToMainMenu.h"
#include "EnhancedInputComponent.h"
#include "CppMultiShooter/CustomTypes/Announcement.h"

void AShooterPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
    ClientElimAnnouncement(Attacker, Victim);
}

void AShooterPlayerController::BeginPlay()
{
    Super::BeginPlay();

    ShooterHUD = Cast<AShooterHUD>(GetHUD());
    
    ServerCheckMatchState();    
}

void AShooterPlayerController::OnPossess(APawn* InPawn) // call by server on possess
{
    Super::OnPossess(InPawn);
    AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(InPawn);
    if (ShooterCharacter)
    {
        InitHUD(ShooterCharacter);
    }
}

void AShooterPlayerController::OnRep_Pawn() // call by client on possess
{
    AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(GetPawn());
    if (ShooterCharacter)
    {
        InitHUD(ShooterCharacter);
    }
}

void AShooterPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    SetHUDTime();
    CheckTimeSync(DeltaTime);
    CheckPing(DeltaTime);
}

void AShooterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AShooterPlayerController, MatchState);
    DOREPLIFETIME(AShooterPlayerController, bShowTeamScores);
}

void AShooterPlayerController::HideTeamScores()
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    bool bHUDValid = ShooterHUD &&
        ShooterHUD->CharacterOverlay &&
        ShooterHUD->CharacterOverlay->RedTeamScore &&
        ShooterHUD->CharacterOverlay->BlueTeamScore &&
        ShooterHUD->CharacterOverlay->ScoreSpacerText;
    if (bHUDValid)
    {
        ShooterHUD->CharacterOverlay->RedTeamScore->SetText(FText());
        ShooterHUD->CharacterOverlay->BlueTeamScore->SetText(FText());
        ShooterHUD->CharacterOverlay->ScoreSpacerText->SetText(FText());
    }
}

void AShooterPlayerController::InitTeamScores()
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    bool bHUDValid = ShooterHUD &&
        ShooterHUD->CharacterOverlay &&
        ShooterHUD->CharacterOverlay->RedTeamScore &&
        ShooterHUD->CharacterOverlay->BlueTeamScore &&
        ShooterHUD->CharacterOverlay->ScoreSpacerText;
    if (bHUDValid)
    {
        FString Zero("0");
        FString Spacer("vs");
        ShooterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(Zero));
        ShooterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(Zero));
        ShooterHUD->CharacterOverlay->ScoreSpacerText->SetText(FText::FromString(Spacer));
    }
}

void AShooterPlayerController::SetHUDRedTeamScore(int32 RedScore)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    bool bHUDValid = ShooterHUD &&
        ShooterHUD->CharacterOverlay &&
        ShooterHUD->CharacterOverlay->RedTeamScore;
    if (bHUDValid)
    {
        FString ScoreText = FString::Printf(TEXT("%d"), RedScore);
        ShooterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(ScoreText));
    }
}

void AShooterPlayerController::SetHUDBlueTeamScore(int32 BlueScore)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    bool bHUDValid = ShooterHUD &&
        ShooterHUD->CharacterOverlay &&
        ShooterHUD->CharacterOverlay->BlueTeamScore;
    if (bHUDValid)
    {
        FString ScoreText = FString::Printf(TEXT("%d"), BlueScore);
        ShooterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(ScoreText));
    }
}

void AShooterPlayerController::PawnLeavingGame()
{
    AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(GetPawn());    
    if (ShooterCharacter && !ShooterCharacter->bLeftGame)
    {
        //UE_LOG(LogTemp, Warning, TEXT("PawnLeavingGame"));
        ShooterCharacter->Elim(true);
    }

    Super::PawnLeavingGame();
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
        FString AmmoText = Ammo == -1 ? "-" : FString::Printf(TEXT("%d"), Ammo);
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
        FString AmmoText = Ammo == -1 ? "-" : FString::Printf(TEXT("%d"), Ammo);
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
}

void AShooterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    bool bHUDValid = ShooterHUD &&
        ShooterHUD->CharacterOverlay &&
        ShooterHUD->CharacterOverlay->ShieldBar &&
        ShooterHUD->CharacterOverlay->ShieldText;
    if (bHUDValid)
    {
        const float ShieldPercent = Shield / MaxShield;
        ShooterHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
        FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
        ShooterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
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
            //or -> LevelStartingTime = ShooterGameMode->GetLevelStartingTime();
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

void AShooterPlayerController::SetHUDGrenades(int32 Grenades)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    bool bHUDValid = ShooterHUD &&
        ShooterHUD->CharacterOverlay &&
        ShooterHUD->CharacterOverlay->GrenadesText;
    if (bHUDValid)
    {
        FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
        ShooterHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
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

void AShooterPlayerController::CheckTimeSync(float DeltaTime)
{
    TimeSyncRunningTime += DeltaTime;
    if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
    {
        ServerRequestServerTime(GetWorld()->GetTimeSeconds());
        TimeSyncRunningTime = 0.f;
    }
}

void AShooterPlayerController::CheckPing(float DeltaTime)
{    
    if (PlayerState == nullptr) return;        

    // this is different than his which is PlayerState->GetPing() * 4 because it is compressed or if(PlayerState->GetPingInMilliseconds() > HighPingThreshold)
    float CurrentPing = PlayerState->GetPingInMilliseconds();
    if (ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->PingAmountText)
    {
        FString PingText = FString::Printf(TEXT("%d"), FMath::FloorToInt(CurrentPing));        
        ShooterHUD->CharacterOverlay->PingAmountText->SetText(FText::FromString(PingText));

        FColor color = FColor::White;
        if (HighPingThreshold < CurrentPing) color = FColor::Red;
        ShooterHUD->CharacterOverlay->SetPingUIColor(color);
  
    }
}

void AShooterPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
    APlayerState* Self = GetPlayerState<APlayerState>();
    if (Attacker && Victim && Self)
    {
        ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
        if (ShooterHUD)
        {
            if (Attacker == Self && Victim != Self)
            {
                ShooterHUD->AddElimAnnouncement("You", Victim->GetPlayerName());
                return;
            }
            if (Victim == Self && Attacker != Self)
            {
                ShooterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "you");
                return;
            }
            if (Attacker == Victim && Attacker == Self)
            {
                ShooterHUD->AddElimAnnouncement("You", "yourself");
                return;
            }
            if (Attacker == Victim && Attacker != Self)
            {
                ShooterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "themselves");
                return;
            }
            ShooterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
        }
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
    SingleTripTime = 0.5f * RoundTripTime;
    float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
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

void AShooterPlayerController::OnMatchStateSet(FName State, bool bTeamsMatch)
{
    MatchState = State;

    if (MatchState == MatchState::InProgress)
    {
        HandleMatchHasStarted(bTeamsMatch);
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

void AShooterPlayerController::InitHUD(AShooterCharacter* ShooterCharacter)
{
    ShooterCharacter->UpdateHUDHealth();
    ShooterCharacter->UpdateHUDShield();
    ShooterCharacter->UpdateHUDAmmo();
    ShooterCharacter->UpdateHUDGrenade();    

    SetHUDScore(HUDScore);
    SetHUDDefeats(HUDDefeats);

}

void AShooterPlayerController::HandleMatchHasStarted(bool bTeamsMatch)
{
    if (HasAuthority()) bShowTeamScores = bTeamsMatch;

    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;
    if (ShooterHUD)
    {
        ShooterHUD->AddCharacterOverlay();
        if (ShooterHUD->Announcement)
        {            
            ShooterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
        }

        if (!HasAuthority()) return;
        if (bTeamsMatch)
        {
            InitTeamScores();
        }
        else
        {
            HideTeamScores();
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
            FString AnnouncementText = Announcement::NewMatchStartsIn;
            ShooterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
            
            // set text top players
            AShooterGameState* ShooterGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
            AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>();
            if (ShooterGameState && ShooterPlayerState)
            {
                TArray<AShooterPlayerState*> TopPlayers = ShooterGameState->TopScoringPlayers;
                FString InfoTextString = bShowTeamScores ? GetTeamsInfoText(ShooterGameState) : GetInfoText(TopPlayers);
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

void AShooterPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent == nullptr) return;
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        EnhancedInputComponent->BindAction(QuitAction, ETriggerEvent::Completed, this, &AShooterPlayerController::ShowReturnToMainMenu);
    }
}

void AShooterPlayerController::ShowReturnToMainMenu()
{
    if (ReturnToMainMenuWidget == nullptr) return;
    if (ReturnToMainMenu == nullptr)
    {
        ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
    }
    if (ReturnToMainMenu)
    {
        bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
        if (bReturnToMainMenuOpen)
        {
            ReturnToMainMenu->MenuSetup();
        }
        else
        {
            ReturnToMainMenu->MenuTearDown();
        }
    }
}

void AShooterPlayerController::OnRep_ShowTeamScores()
{
    if (bShowTeamScores)
    {
        InitTeamScores();
    }
    else
    {
        HideTeamScores();
    }
}

void AShooterPlayerController::SetHUDScope(bool bIsAiming, TSubclassOf<UScopeWidget> ScopeClass)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterHUD>(GetHUD()) : ShooterHUD;

    //UE_LOG(LogTemp, Warning, TEXT("bIsAiming: %d"), bIsAiming);
    bool bHUDValid = ShooterHUD &&
        ShooterHUD->ScopeWidget &&
        ShooterHUD->ScopeWidget->ScopeZoomIn;
    //UE_LOG(LogTemp, Warning, TEXT("bIsAiming: %d"), bIsAiming);

    if (!ShooterHUD->ScopeWidget)
    {
        ShooterHUD->AddScopeWidget(ScopeClass);
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

FString AShooterPlayerController::GetInfoText(const TArray<class AShooterPlayerState*>& Players)
{
    AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>();
    if (ShooterPlayerState == nullptr) return FString();
    FString InfoTextString;
    if (Players.Num() == 0)
    {
        InfoTextString = Announcement::ThereIsNoWinner;
    }
    else if (Players.Num() == 1 && Players[0] == ShooterPlayerState)
    {
        InfoTextString = Announcement::YouAreTheWinner;
    }
    else if (Players.Num() == 1)
    {
        InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *Players[0]->GetPlayerName());
    }
    else if (Players.Num() > 1)
    {
        InfoTextString = Announcement::PlayersTiedForTheWin;
        InfoTextString.Append(FString("\n"));
        for (auto TiedPlayer : Players)
        {
            InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
        }
    }

    return InfoTextString;
}

FString AShooterPlayerController::GetTeamsInfoText(AShooterGameState* ShooterGameState)
{
    if (ShooterGameState == nullptr) return FString();
    FString InfoTextString;

    const int32 RedTeamScore = ShooterGameState->RedTeamScore;
    const int32 BlueTeamScore = ShooterGameState->BlueTeamScore;

    if (RedTeamScore == 0 && BlueTeamScore == 0)
    {
        InfoTextString = Announcement::ThereIsNoWinner;
    }
    else if (RedTeamScore == BlueTeamScore)
    {
        InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::TeamsTiedForTheWin);
        InfoTextString.Append(Announcement::RedTeam);
        InfoTextString.Append(TEXT("\n"));
        InfoTextString.Append(Announcement::BlueTeam);
        InfoTextString.Append(TEXT("\n"));
    }
    else if (RedTeamScore > BlueTeamScore)
    {
        InfoTextString = Announcement::RedTeamWins;
        InfoTextString.Append(TEXT("\n"));
        InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::RedTeam, RedTeamScore));
        InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::BlueTeam, BlueTeamScore));
    }
    else if (BlueTeamScore > RedTeamScore)
    {
        InfoTextString = Announcement::BlueTeamWins;
        InfoTextString.Append(TEXT("\n"));
        InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::BlueTeam, BlueTeamScore));
        InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::RedTeam, RedTeamScore));
    }

    return InfoTextString;
}