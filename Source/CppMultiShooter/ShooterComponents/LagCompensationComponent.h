// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

USTRUCT(BlueprintType)
struct FBoxInformation
{
    GENERATED_BODY()

    UPROPERTY()
    FVector Location;

    UPROPERTY()
    FRotator Rotation;

    UPROPERTY()
    FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
    GENERATED_BODY()

    UPROPERTY()
    float Time;

    UPROPERTY()
    TMap<FName, FBoxInformation> HitBoxInfo;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
    GENERATED_BODY()

    UPROPERTY()
    bool bHitConfirmed;

    UPROPERTY()
    bool bHeadShot;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CPPMULTISHOOTER_API ULagCompensationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULagCompensationComponent();
    friend class AShooterCharacter;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void ShowFramePackage(const FFramePackage& Package, const FColor& Color);
    FServerSideRewindResult  ServerSideRewind(
        class AShooterCharacter* HitCharacter,
        const FVector_NetQuantize& TraceStart,
        const FVector_NetQuantize& HitLocation,
        float HitTime);

    UFUNCTION(Server, Reliable)
    void ServerScoreRequest(
        AShooterCharacter* HitCharacter,
        const FVector_NetQuantize& TraceStart,
        const FVector_NetQuantize& HitLocation,
        float HitTime,
        class AWeapon* DamageCauser
    );

protected:
    virtual void BeginPlay() override;
    void SaveFramePackage(FFramePackage& Package);
    void SaveFramePackage();
    FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);

    FServerSideRewindResult ConfirmHit(
        const FFramePackage& Package,
        AShooterCharacter* HitCharacter,
        const FVector_NetQuantize& TraceStart,
        const FVector_NetQuantize& HitLocation);
    void CacheBoxPositions(AShooterCharacter* HitCharacter, FFramePackage& OutFramePackage);
    void MoveBoxes(AShooterCharacter* HitCharacter, const FFramePackage& Package);
    void ResetHitBoxes(AShooterCharacter* HitCharacter, const FFramePackage& Package);
    void EnableCharacterMeshCollision(AShooterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);

private:

    UPROPERTY()
    AShooterCharacter* Character;

    UPROPERTY()
    class AShooterPlayerController* Controller;

    TDoubleLinkedList<FFramePackage> FrameHistory;

    UPROPERTY(EditAnywhere)
    float MaxRecordTime = 4.f;

public:


};