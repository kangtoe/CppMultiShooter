// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "CppMultiShooter/Weapon/Weapon.h"
#include "Kismet/GameplayStatics.h"
#include "CppMultiShooter/CppMultiShooter.h"

ULagCompensationComponent::ULagCompensationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

}

void ULagCompensationComponent::BeginPlay()
{
    Super::BeginPlay();   
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    SaveFramePackage();
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package) // 시간과 히트박스 정보 저장
{
    Character = Character == nullptr ? Cast<AShooterCharacter>(GetOwner()) : Character;
    if (Character)
    {
        Package.Time = GetWorld()->GetTimeSeconds();
        Package.Character = Character;
        for (auto& BoxPair : Character->GetHitCollisionBoxes())
        {
            FBoxInformation BoxInformation;
            BoxInformation.Location = BoxPair.Value->GetComponentLocation();
            BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
            BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();
            Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
        }
    }
}

void ULagCompensationComponent::SaveFramePackage() // 시간과 히트박스 정보 저장 + 데이터 정리 (삭제&정렬)
{
    if (Character == nullptr || !Character->HasAuthority()) return;
    if (FrameHistory.Num() <= 1)
    {
        FFramePackage ThisFrame;
        SaveFramePackage(ThisFrame);
        FrameHistory.AddHead(ThisFrame);
    }
    else
    {
        float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
        while (HistoryLength > MaxRecordTime)
        {
            FrameHistory.RemoveNode(FrameHistory.GetTail());
            HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
        }
        FFramePackage ThisFrame;
        SaveFramePackage(ThisFrame);
        FrameHistory.AddHead(ThisFrame);

        //ShowFramePackage(ThisFrame, FColor::Red);
    }
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
    const float Distance = YoungerFrame.Time - OlderFrame.Time;
    const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);

    FFramePackage InterpFramePackage;
    InterpFramePackage.Time = HitTime;

    for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
    {
        const FName& BoxInfoName = YoungerPair.Key;

        const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
        const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];

        FBoxInformation InterpBoxInfo;

        InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
        InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
        InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;

        InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
    }

    return InterpFramePackage;
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations)
{
    for (auto& Frame : FramePackages)
    {
        if (Frame.Character == nullptr) return FShotgunServerSideRewindResult();
    }

    FShotgunServerSideRewindResult ShotgunResult;
    TArray<FFramePackage> CurrentFrames;
    for (auto& Frame : FramePackages)
    {
        FFramePackage CurrentFrame;
        CurrentFrame.Character = Frame.Character;
        CacheBoxPositions(Frame.Character, CurrentFrame);
        MoveBoxes(Frame.Character, Frame);
        EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);
        CurrentFrames.Add(CurrentFrame);
    }

    for (auto& Frame : FramePackages)
    {
        // Enable collision for the head first
        UBoxComponent* HeadBox = Frame.Character->GetHitCollisionBoxes()[FName("head")];
        HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
    }

    UWorld* World = GetWorld();
    // check for head shots
    for (auto& HitLocation : HitLocations)
    {
        FHitResult ConfirmHitResult;
        const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
        if (World)
        {
            World->LineTraceSingleByChannel(
                ConfirmHitResult,
                TraceStart,
                TraceEnd,
                ECC_HitBox
            );
            AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(ConfirmHitResult.GetActor());
            if (ShooterCharacter)
            {
                if (ShotgunResult.HeadShots.Contains(ShooterCharacter))
                {
                    ShotgunResult.HeadShots[ShooterCharacter]++;
                }
                else
                {
                    ShotgunResult.HeadShots.Emplace(ShooterCharacter, 1);
                }

                // for head shot debug
                if (ConfirmHitResult.Component.IsValid())
                {
                    UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
                    if (Box)
                    {
                        DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
                    }
                }
            }            
        }
    }

    // enable collision for all boxes, then disable for head box
    for (auto& Frame : FramePackages)
    {
        for (auto& HitBoxPair : Frame.Character->GetHitCollisionBoxes())
        {
            if (HitBoxPair.Value != nullptr)
            {
                HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
            }
        }
        UBoxComponent* HeadBox = Frame.Character->GetHitCollisionBoxes()[FName("head")];
        HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // check for body shots
    for (auto& HitLocation : HitLocations)
    {
        FHitResult ConfirmHitResult;
        const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
        if (World)
        {
            World->LineTraceSingleByChannel(
                ConfirmHitResult,
                TraceStart,
                TraceEnd,
                ECC_HitBox
            );
            AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(ConfirmHitResult.GetActor());
            if (ShooterCharacter)
            {                
                if (ShotgunResult.BodyShots.Contains(ShooterCharacter))
                {
                    ShotgunResult.BodyShots[ShooterCharacter]++;
                }
                else
                {
                    ShotgunResult.BodyShots.Emplace(ShooterCharacter, 1);
                }

                // for body shot debug
                if (ConfirmHitResult.Component.IsValid())
                {
                    UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
                    if (Box)
                    {
                        DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Blue, false, 8.f);
                    }
                }
            }
        }
    }

    for (auto& Frame : CurrentFrames)
    {
        ResetHitBoxes(Frame.Character, Frame);
        EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
    }

    return ShotgunResult;
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package, AShooterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
    FFramePackage CurrentFrame;
    CacheBoxPositions(HitCharacter, CurrentFrame);
    MoveBoxes(HitCharacter, Package);
    EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

    // Enable collision for the head first
    UBoxComponent* HeadBox = HitCharacter->GetHitCollisionBoxes()[FName("head")];
    HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

    FPredictProjectilePathParams PathParams;
    PathParams.bTraceWithCollision = true;
    PathParams.MaxSimTime = MaxRecordTime;
    PathParams.LaunchVelocity = InitialVelocity;
    PathParams.StartLocation = TraceStart;
    PathParams.SimFrequency = 15.f;
    PathParams.ProjectileRadius = 5.f;
    PathParams.TraceChannel = ECC_HitBox;
    PathParams.ActorsToIgnore.Add(GetOwner());
    PathParams.DrawDebugTime = 5.f;
    PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;

    FPredictProjectilePathResult PathResult;
    UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

    if (PathResult.HitResult.bBlockingHit) // we hit the head, return early
    {
        if (PathResult.HitResult.Component.IsValid())
        {
            UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component);
            if (Box)
            {
                DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
            }
        }

        ResetHitBoxes(HitCharacter, CurrentFrame);
        EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
        return FServerSideRewindResult{ true, true };
    }
    else // we didn't hit the head; check the rest of the boxes
    {
        for (auto& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
        {
            if (HitBoxPair.Value != nullptr)
            {
                HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
            }
        }

        UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
        if (PathResult.HitResult.bBlockingHit)
        {
            if (PathResult.HitResult.Component.IsValid())
            {
                UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component);
                if (Box)
                {
                    DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Blue, false, 8.f);
                }
            }

            ResetHitBoxes(HitCharacter, CurrentFrame);
            EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
            return FServerSideRewindResult{ true, false };
        }
    }

    ResetHitBoxes(HitCharacter, CurrentFrame);
    EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
    return FServerSideRewindResult{ false, false };
}

void ULagCompensationComponent::CacheBoxPositions(AShooterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
    if (HitCharacter == nullptr) return;
    for (const TTuple<FName, UBoxComponent*>& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
    {
        if (HitBoxPair.Value != nullptr)
        {
            FBoxInformation BoxInfo;
            BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
            BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
            BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
            OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
        }
    }
}

void ULagCompensationComponent::MoveBoxes(AShooterCharacter* HitCharacter, const FFramePackage& Package)
{
    if (HitCharacter == nullptr) return;

    for (const TTuple<FName, UBoxComponent*>& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
    {
        if (HitBoxPair.Value != nullptr)
        {
            const FBoxInformation* BoxValue = Package.HitBoxInfo.Find(HitBoxPair.Key);

            if (BoxValue)
            {
                HitBoxPair.Value->SetWorldLocation(BoxValue->Location);
                HitBoxPair.Value->SetWorldRotation(BoxValue->Rotation);
                HitBoxPair.Value->SetBoxExtent(BoxValue->BoxExtent);
            }

        }
    }
}

void ULagCompensationComponent::ResetHitBoxes(AShooterCharacter* HitCharacter, const FFramePackage& Package)
{
    if (HitCharacter == nullptr) return;

    for (const TTuple<FName, UBoxComponent*>& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
    {
        if (HitBoxPair.Value != nullptr)
        {
            const FBoxInformation* BoxValue = Package.HitBoxInfo.Find(HitBoxPair.Key);

            if (BoxValue)
            {
                HitBoxPair.Value->SetWorldLocation(BoxValue->Location);
                HitBoxPair.Value->SetWorldRotation(BoxValue->Rotation);
                HitBoxPair.Value->SetBoxExtent(BoxValue->BoxExtent);
                HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            }
        }
    }
}

void ULagCompensationComponent::EnableCharacterMeshCollision(AShooterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
    if (HitCharacter && HitCharacter->GetMesh())
    {
        HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
    }
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
    for (auto& BoxInfo : Package.HitBoxInfo)
    {
        DrawDebugBox(
            GetWorld(),
            BoxInfo.Value.Location,
            BoxInfo.Value.BoxExtent,
            FQuat(BoxInfo.Value.Rotation),
            Color,
            false,
            MaxRecordTime
        );
    }
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(AShooterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
    FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
    return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity, HitTime);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<AShooterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
    TArray<FFramePackage> FramesToCheck;
    for (AShooterCharacter* HitCharacter : HitCharacters)
    {
        FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
    }

    return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(AShooterCharacter* HitCharacter, float HitTime)
{
    bool bReturn =
        HitCharacter == nullptr ||
        HitCharacter->GetLagCompensation() == nullptr ||
        HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr ||
        HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;
    if (bReturn) return FFramePackage();

    // Frame package that we check to verify a hit
    FFramePackage FrameToCheck;
    bool bShouldInterpolate = true;

    // Frame history of the HitCharacter
    const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
    const float OldestHistoryTime = History.GetTail()->GetValue().Time;
    const float NewestHistoryTime = History.GetHead()->GetValue().Time;
    if (OldestHistoryTime > HitTime)
    {
        // too far back - too laggy to do SSR
        return FFramePackage();
    }
    if (OldestHistoryTime == HitTime)
    {
        FrameToCheck = History.GetTail()->GetValue();
        bShouldInterpolate = false;
    }
    if (NewestHistoryTime <= HitTime)
    {
        FrameToCheck = History.GetHead()->GetValue();
        bShouldInterpolate = false;
    }

    // March back until: OlderTime < HitTime < YoungerTime
    auto Older = History.GetHead();
    for (auto It = begin(History); It; ++It)
    {
        auto CurrentFrame = *It;
        if (CurrentFrame.Time <= HitTime) // is Older still younger than HitTime?
        {
            Older = It.GetNode();
        }
    }
    auto Younger = Older->GetPrevNode();

    if (Older->GetValue().Time == HitTime) // highly unlikely, but we found our frame to check
    {
        FrameToCheck = Older->GetValue();
        bShouldInterpolate = false;
    }
    if (bShouldInterpolate)
    {
        // Interpolate between Younger and Older
        FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
    }

    return FrameToCheck;
}

void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(AShooterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
    FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);

    if (Character && HitCharacter && Confirm.bHitConfirmed)
    {
        UGameplayStatics::ApplyDamage(
            HitCharacter,
            Character->GetEquippedWeapon()->GetDamage(),
            Character->Controller,
            Character->GetEquippedWeapon(),
            UDamageType::StaticClass()
        );
    }
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(const TArray<AShooterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
    FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

    for (auto& HitCharacter : HitCharacters)
    {
        if (!Character || !Character->GetEquippedWeapon()) return;
        AWeapon* weapon = Character->GetEquippedWeapon();        

        if (HitCharacter == nullptr) continue;
        float TotalDamage = 0.f;
        if (Confirm.HeadShots.Contains(HitCharacter))
        {
            float HeadShotDamage = Confirm.HeadShots[HitCharacter] * weapon->GetDamage();
            TotalDamage += HeadShotDamage;
        }
        if (Confirm.BodyShots.Contains(HitCharacter))
        {
            float BodyShotDamage = Confirm.BodyShots[HitCharacter] * weapon->GetDamage();
            TotalDamage += BodyShotDamage;
        }
        UGameplayStatics::ApplyDamage(
            HitCharacter,
            TotalDamage,
            Character->Controller,
            weapon,
            UDamageType::StaticClass()
        );
    }
}
