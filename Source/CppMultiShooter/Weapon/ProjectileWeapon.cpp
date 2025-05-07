// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const TArray<FVector_NetQuantize>& HitTargets)
{
    Super::Fire(HitTargets);

    if (!HasAuthority()) return;

    APawn* InstigatorPawn = Cast<APawn>(GetOwner());
    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
    if (MuzzleFlashSocket)
    {
        // 발사체 위치 정보 구하기
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

		for (FVector_NetQuantize HitTarget : HitTargets)
		{
            // From muzzle flash socket, to hit location from TraceUnderCrosshairs
            FVector ToTarget = HitTarget - SocketTransform.GetLocation();
            FRotator TargetRotation = ToTarget.Rotation();

            if (ProjectileClass && InstigatorPawn)
            {
                // 발사체 생성
                FActorSpawnParameters SpawnParams;
                SpawnParams.Owner = GetOwner();
                SpawnParams.Instigator = InstigatorPawn;
                UWorld* World = GetWorld();
                if (World)
                {
                    AProjectile* prj = World->SpawnActor<AProjectile>(
                        ProjectileClass,
                        SocketTransform.GetLocation(),
                        TargetRotation,
                        SpawnParams
                    );
                    prj->SetDamage(Damage);
                }
            }
		}

    }
}