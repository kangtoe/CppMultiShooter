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
    void SetHUDShield(float Shield, float MaxShield);
    void SetHUDScore(float Score);
    void SetHUDDefeats(int32 Defeats);
    void SetHUDWeaponAmmo(int32 Ammo);
    void SetHUDCarriedAmmo(int32 Ammo);
    void SetHUDMatchCountdown(float CountdownTime);
    void SetHUDAnnouncementCountdown(float CountdownTime);
    void SetHUDGrenades(int32 Grenades);

    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnRep_Pawn() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;    

    virtual float GetServerTime(); // Synced with server world clock
    virtual void ReceivedPlayer() override; // Sync with server clock as soon as possible
    void OnMatchStateSet(FName State);
    void HandleMatchHasStarted();
    void HandleCooldown();

    float SingleTripTime = 0.f;

    void SetHUDScope(bool bIsAiming, TSubclassOf<class UScopeWidget> ScopeClass);

protected:
    virtual void BeginPlay() override;    
    void SetHUDTime();

    /**
    * Sync time between client and server
    */    
    UFUNCTION(Server, Reliable) // Requests the current server time, passing in the client's time when the request was sent
    void ServerRequestServerTime(float TimeOfClientRequest);
    UFUNCTION(Client, Reliable) // Reports the current server time to the client in response to ServerRequestServerTime
    void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);
    float ClientServerDelta = 0.f; // difference between client and server time
    UPROPERTY(EditAnywhere, Category = Time)
    float TimeSyncFrequency = 5.f;
    float TimeSyncRunningTime = 0.f;
    void CheckTimeSync(float DeltaTime);

    UFUNCTION(Server, Reliable)
    void ServerCheckMatchState();

    UFUNCTION(Client, Reliable)
    void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);

    UPROPERTY(EditAnywhere)
    float HighPingThreshold = 250.f;
    void CheckPing(float DeltaTime);

private:
    UPROPERTY()
    class AShooterHUD* ShooterHUD;

    float LevelStartingTime = 0.f;
    float MatchTime = 0.f;
    float WarmupTime = 0.f;
    float CooldownTime = 0.f;
    uint32 CountdownInt = 0;

    UPROPERTY(ReplicatedUsing = OnRep_MatchState)
    FName MatchState;

    UFUNCTION()
    void OnRep_MatchState();

    UPROPERTY()
    class UCharacterOverlay* CharacterOverlay;    

    float HUDScore;
    int32 HUDDefeats;

    void InitHUD(class AShooterCharacter* ShooterCharacter);
};