// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "WeaponTypes.h"

#include "DrawDebugHelpers.h"

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

        TMap<AShooterCharacter*, uint32> HitMap;
        for (FVector_NetQuantize HitTarget : HitTargets)
        {          
            FHitResult FireHit;
            WeaponTraceHit(Start, HitTarget, FireHit);

            AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(FireHit.GetActor());
            if (ShooterCharacter)
            {
                if (HitMap.Contains(ShooterCharacter))
                {
                    HitMap[ShooterCharacter]++;
                }
                else
                {
                    HitMap.Emplace(ShooterCharacter, 1);
                }
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
        for (auto HitPair : HitMap)
        {
            if (HitPair.Key && HasAuthority() && InstigatorController)
            {
                UGameplayStatics::ApplyDamage(
                    HitPair.Key, // Character that was hit
                    Damage * HitPair.Value, // Multiply Damage by number of times hit
                    InstigatorController,
                    this,
                    UDamageType::StaticClass()
                );
            }

        }
    }
}

/*
void AHitScanWeapon::Fire(const FVector& HitTarget)
{
    Super::Fire(HitTarget);

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn == nullptr) return;
    AController* InstigatorController = OwnerPawn->GetController();

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (MuzzleFlashSocket)
    {
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        FVector Start = SocketTransform.GetLocation();
        
        FHitResult FireHit;
        WeaponTraceHit(Start, HitTarget, FireHit);
        

        AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(FireHit.GetActor());
        if (ShooterCharacter && HasAuthority() && InstigatorController)        
        {
            UGameplayStatics::ApplyDamage(
                ShooterCharacter,
                Damage,
                InstigatorController,
                this,
                UDamageType::StaticClass()
            );
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
                    FireHit.ImpactPoint
                );
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
    }
}
*/

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
