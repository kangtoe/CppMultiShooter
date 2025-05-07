// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "CppMultiShooter/Weapon/Weapon.h"
#include "Kismet/GameplayStatics.h"

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

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, AShooterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
    if (HitCharacter == nullptr) return FServerSideRewindResult();

    FFramePackage CurrentFrame;
    CacheBoxPositions(HitCharacter, CurrentFrame); // 현재 캐릭터의 히트박스 상태 백업
    MoveBoxes(HitCharacter, Package); // 과거 위치로 히트박스 이동 (리와인드)
    EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision); // 메시 충돌 비활성화 (히트박스 충돌만 사용)

    // 우선적으로 'head' 박스에만 충돌 활성화
    UBoxComponent* HeadBox = HitCharacter->GetHitCollisionBoxes()[FName("head")];
    HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HeadBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

    FHitResult ConfirmHitResult;
    const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
    UWorld* World = GetWorld();
    if (World)
    {
        World->LineTraceSingleByChannel( // 첫 번째 라인트레이스로 헤드샷 여부 확인
            ConfirmHitResult,
            TraceStart,
            TraceEnd,
            ECollisionChannel::ECC_Visibility
        );
        if (ConfirmHitResult.bBlockingHit) // 헤드에 명중: 히트박스 복구 및 메시 충돌 복원 후 헤드샷 반환
        {
            ResetHitBoxes(HitCharacter, CurrentFrame);
            EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);
            return FServerSideRewindResult{ true, true };
        }
        else // 헤드샷이 아니므로 모든 히트박스에 충돌 활성화
        {
            for (auto& HitBoxPair : HitCharacter->GetHitCollisionBoxes()) 
            {
                if (HitBoxPair.Value != nullptr)
                {
                    HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                    HitBoxPair.Value->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
                }
            }
            World->LineTraceSingleByChannel( // 두 번째 라인트레이스로 몸통 등 다른 부위 명중 확인
                ConfirmHitResult,
                TraceStart,
                TraceEnd,
                ECollisionChannel::ECC_Visibility
            );
            if (ConfirmHitResult.bBlockingHit)  // 명중 시: 히트박스 복구 및 메시 충돌 복원 후 바디샷 반환
            {
                ResetHitBoxes(HitCharacter, CurrentFrame);
                EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);
                return FServerSideRewindResult{ true, false };
            }
        }
    }

    // 어떤 부위에도 명중하지 않음: 복구 후 실패 반환
    ResetHitBoxes(HitCharacter, CurrentFrame);
    EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);
    return FServerSideRewindResult{ false, false };
}

void ULagCompensationComponent::CacheBoxPositions(AShooterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
    if (HitCharacter == nullptr) return;
    for (auto& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
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
    for (auto& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
    {
        if (HitBoxPair.Value != nullptr)
        {
            HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
            HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
            HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
        }
    }
}

void ULagCompensationComponent::ResetHitBoxes(AShooterCharacter* HitCharacter, const FFramePackage& Package)
{
    if (HitCharacter == nullptr) return;
    for (auto& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
    {
        if (HitBoxPair.Value != nullptr)
        {
            HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
            HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
            HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
            HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(AShooterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
    bool bReturn =
        HitCharacter == nullptr ||
        HitCharacter->GetLagCompensation() == nullptr ||
        HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr ||
        HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;
    if (bReturn) return;

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
        return;
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

    if (bReturn) return;
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(AShooterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, AWeapon* DamageCauser)
{
    FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

    if (Character && HitCharacter && DamageCauser && Confirm.bHitConfirmed)
    {
        UGameplayStatics::ApplyDamage(
            HitCharacter,
            DamageCauser->GetDamage(),
            Character->Controller,
            DamageCauser,
            UDamageType::StaticClass()
        );
    }
}
