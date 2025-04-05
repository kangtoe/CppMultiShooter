// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterGameMode.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "CppMultiShooter/PlayerController/ShooterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "CppMultiShooter/PlayerState/ShooterPlayerState.h"

AShooterGameMode::AShooterGameMode()
{
    bDelayedStart = true;
}

void AShooterGameMode::BeginPlay()
{
    Super::BeginPlay();

    LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void AShooterGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (MatchState == MatchState::WaitingToStart)
    {
        CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
        if (CountdownTime <= 0.f)
        {
            StartMatch();
        }
    }
}

void AShooterGameMode::PlayerEliminated(AShooterCharacter* ElimmedCharacter, AShooterPlayerController* VictimController, AShooterPlayerController* AttackerController)
{
    if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
    if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
    AShooterPlayerState* AttackerPlayerState = AttackerController ? Cast<AShooterPlayerState>(AttackerController->PlayerState) : nullptr;
    AShooterPlayerState* VictimPlayerState = VictimController ? Cast<AShooterPlayerState>(VictimController->PlayerState) : nullptr;

    if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
    {
        AttackerPlayerState->AddToScore(1.f);
    }
    if (VictimPlayerState)
    {
        VictimPlayerState->AddToDefeats(1);
    }

    if (ElimmedCharacter)
    {
        ElimmedCharacter->Elim();
    }
}

void AShooterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
    if (ElimmedCharacter)
    {
        ElimmedCharacter->Reset();
        ElimmedCharacter->Destroy();
    }
    if (ElimmedController)
    {
        // get random playerStart
        TArray<AActor*> PlayerStarts;
        UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
        int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
        RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
    }
}