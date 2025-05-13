// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "CppMultiShooter/PlayerController/ShooterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "WeaponTypes.h"
#include "CppMultiShooter/ShooterComponents/LagCompensationComponent.h"

//#include "DrawDebugHelpers.h"

void AHitScanWeapon::Fire(const TArray<FVector_NetQuantize>& HitTargets) // shotgun tmp
{ 
    Super::Fire(HitTargets);

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn == nullptr) return;
    AController* InstigatorController = OwnerPawn->GetController();

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (MuzzleFlashSocket)
    {
        const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
 		const FVector Start = SocketTransform.GetLocation();

        // Maps hit character to number of times hit
        TMap<AShooterCharacter*, uint32> HitMap;
        TMap<AShooterCharacter*, uint32> HeadShotHitMap;
        for (const FVector_NetQuantize& HitTarget : HitTargets)
        {
            FHitResult FireHit;
            WeaponTraceHit(Start, HitTarget, FireHit);

            AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(FireHit.GetActor());
            if (ShooterCharacter)
            {
                const bool bHeadShot = FireHit.BoneName.ToString() == FString("head");

                if (bHeadShot)
                {
                    if (HeadShotHitMap.Contains(ShooterCharacter)) HeadShotHitMap[ShooterCharacter]++;
                    else HeadShotHitMap.Emplace(ShooterCharacter, 1);
                }
                else
                {
                    if (HitMap.Contains(ShooterCharacter)) HitMap[ShooterCharacter]++;
                    else HitMap.Emplace(ShooterCharacter, 1);
                }

                if (ImpactParticles)
                {
                    UGameplayStatics::SpawnEmitterAtLocation(
                        GetWorld(),
                        ImpactParticles,
                        FireHit.ImpactPoint,
                        FireHit.ImpactNormal.Rotation()
                    );
                }
                if (HitSound)
                {
                    UGameplayStatics::PlaySoundAtLocation(
                        this,
                        HitSound,
                        FireHit.ImpactPoint,
                        .5f,
                        FMath::FRandRange(-.5f, .5f)
                    );
                }
            }
        }

        if (MuzzleFlash)
        {
            UGameplayStatics::SpawnEmitterAtLocation(
                GetWorld(),
                MuzzleFlash,
                SocketTransform
            );
        }
        if (FireSound)
        {
            UGameplayStatics::PlaySoundAtLocation(
                this,
                FireSound,
                GetActorLocation()
            );
        }

        TArray<AShooterCharacter*> HitCharacters;        
        TMap<AShooterCharacter*, float> DamageMap; // Maps Character hit to total damage     
        
        // 바디샷 계산 Calculate body shot damage by multiplying times hit x Damage - store in DamageMap
        for (auto& HitPair : HitMap)
        {
            if (HitPair.Key)
            {
                DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage);

                HitCharacters.AddUnique(HitPair.Key);
            }
        }
        // 헤드샷 계산 Calculate head shot damage by multiplying times hit x HeadShotDamage - store in DamageMap
        for (auto& HeadShotHitPair : HeadShotHitMap)
        {
            if (HeadShotHitPair.Key)
            {
                if (DamageMap.Contains(HeadShotHitPair.Key)) DamageMap[HeadShotHitPair.Key] += HeadShotHitPair.Value * HeadShotDamage;
                else DamageMap.Emplace(HeadShotHitPair.Key, HeadShotHitPair.Value * HeadShotDamage);

                HitCharacters.AddUnique(HeadShotHitPair.Key);
            }
        }
        // 총 데미지 계산 Loop through DamageMap to get total damage for each character
        for (auto& DamagePair : DamageMap)
        {
            if (DamagePair.Key && InstigatorController)
            {
                bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
                if (HasAuthority() && bCauseAuthDamage)
                {
                    UGameplayStatics::ApplyDamage(
                        DamagePair.Key, // Character that was hit
                        DamagePair.Value, // Damage calculated in the two for loops above
                        InstigatorController,
                        this,
                        UDamageType::StaticClass()
                    );
                }
            }
        }
        // 서버가 아닌 클라이언트면 SSR
        if (!HasAuthority() && bUseServerSideRewind)
        {
            ShooterOwnerCharacter = ShooterOwnerCharacter == nullptr ? Cast<AShooterCharacter>(OwnerPawn) : ShooterOwnerCharacter;
            ShooterOwnerController = ShooterOwnerController == nullptr ? Cast<AShooterPlayerController>(InstigatorController) : ShooterOwnerController;            
            if (ShooterOwnerController && ShooterOwnerCharacter && ShooterOwnerCharacter->GetLagCompensation() && ShooterOwnerCharacter->IsLocallyControlled())
            {
                ShooterOwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
                    HitCharacters,
                    Start,
                    HitTargets,
                    ShooterOwnerController->GetServerTime() - ShooterOwnerController->SingleTripTime
                );
            }
        }
    }
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
    UWorld* World = GetWorld();
    if (World)
    {
        FVector End = HitTarget;

        World->LineTraceSingleByChannel(
            OutHit,
            TraceStart,
            End,
            ECollisionChannel::ECC_Visibility
        );
        FVector BeamEnd = End;
        if (OutHit.bBlockingHit)
        {
            BeamEnd = OutHit.ImpactPoint;
        } 
        DrawDebugSphere(GetWorld(), BeamEnd, 16.f, 12, FColor::Orange, true);
        if (BeamParticles)
        {
            UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
                World,
                BeamParticles,
                TraceStart,
                FRotator::ZeroRotator,
                true
            );
            if (Beam)
            {
                Beam->SetVectorParameter(FName("Target"), BeamEnd);
            }
        }
    }
}
