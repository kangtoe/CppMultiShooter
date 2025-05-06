// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"

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

    FFramePackage Package;
    SaveFramePackage(Package);
    ShowFramePackage(Package, FColor::Orange);
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
    Character = Character == nullptr ? Cast<AShooterCharacter>(GetOwner()) : Character;
    if (Character)
    {
        Package.Time = GetWorld()->GetTimeSeconds();
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
            0
        );
    }
}
