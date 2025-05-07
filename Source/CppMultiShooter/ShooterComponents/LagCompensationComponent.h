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

    UPROPERTY()
    AShooterCharacter* Character;
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

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
    GENERATED_BODY()

    UPROPERTY()
    TMap<AShooterCharacter*, uint32> HeadShots;

    UPROPERTY()
    TMap<AShooterCharacter*, uint32> BodyShots;

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

    FShotgunServerSideRewindResult ShotgunServerSideRewind(
        const TArray<AShooterCharacter*>& HitCharacters,
        const FVector_NetQuantize& TraceStart,
        const TArray<FVector_NetQuantize>& HitLocations,
        float HitTime
    );

    UFUNCTION(Server, Reliable)
    void ShotgunServerScoreRequest(
        const TArray<AShooterCharacter*>& HitCharacters,
        const FVector_NetQuantize& TraceStart,
        const TArray<FVector_NetQuantize>& HitLocations,
        float HitTime
    );

protected:
    virtual void BeginPlay() override;
    void SaveFramePackage(FFramePackage& Package);
    void SaveFramePackage();
    FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime); 
    
    FShotgunServerSideRewindResult ShotgunConfirmHit(
        const TArray<FFramePackage>& FramePackages,
        const FVector_NetQuantize& TraceStart,
        const TArray<FVector_NetQuantize>& HitLocations
    );

    void CacheBoxPositions(AShooterCharacter* HitCharacter, FFramePackage& OutFramePackage);
    void MoveBoxes(AShooterCharacter* HitCharacter, const FFramePackage& Package);
    void ResetHitBoxes(AShooterCharacter* HitCharacter, const FFramePackage& Package);
    void EnableCharacterMeshCollision(AShooterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);

    FFramePackage GetFrameToCheck(AShooterCharacter* HitCharacter, float HitTime);

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