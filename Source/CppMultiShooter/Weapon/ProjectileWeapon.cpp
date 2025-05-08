// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const TArray<FVector_NetQuantize>& HitTargets)
{
    Super::Fire(HitTargets);    

    APawn* InstigatorPawn = Cast<APawn>(GetOwner());
    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
    UWorld* World = GetWorld();
	if (MuzzleFlashSocket && World)
    {
        // 발사체 위치 정보 구하기
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

		for (const FVector_NetQuantize& HitTarget : HitTargets)
		{
            // From muzzle flash socket, to hit location from TraceUnderCrosshairs
            FVector ToTarget = HitTarget - SocketTransform.GetLocation();
            FRotator TargetRotation = ToTarget.Rotation();

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;

			AProjectile* SpawnedProjectile = nullptr;
			if (bUseServerSideRewind)
			{				
				if (InstigatorPawn->HasAuthority()) // server
				{
					if (InstigatorPawn->IsLocallyControlled()) // server, host - use replicated projectile
					{
						SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
						SpawnedProjectile->bUseServerSideRewind = false;
						SpawnedProjectile->SetDamage(Damage);
					}
					else // server, not locally controlled - spawn non-replicated projectile, no SSR
					{
						SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
						SpawnedProjectile->bUseServerSideRewind = false;
					}
				}
				else // client, using SSR
				{
					if (InstigatorPawn->IsLocallyControlled()) // client, locally controlled - spawn non-replicated projectile, use SSR
					{
						SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
						SpawnedProjectile->bUseServerSideRewind = true;
						SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
						SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
						SpawnedProjectile->SetDamage(Damage);
					}
					else // client, not locally controlled - spawn non-replicated projectile, no SSR
					{
						SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
						SpawnedProjectile->bUseServerSideRewind = false;
					}
				}

				SpawnedProjectile->bUseServerSideRewind = true;
			}
			else // weapon not using SSR
			{
				if (InstigatorPawn->HasAuthority())
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
					SpawnedProjectile->SetDamage(Damage);
				}
			}
            
		} // end for loop

    }
}