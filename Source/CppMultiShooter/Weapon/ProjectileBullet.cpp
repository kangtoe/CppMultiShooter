// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "CppMultiShooter/PlayerController/ShooterPlayerController.h"
#include "CppMultiShooter/ShooterComponents/LagCompensationComponent.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{    
    AShooterCharacter* OwnerCharacter = Cast<AShooterCharacter>(GetOwner());    
    if (OwnerCharacter)
    {
        AShooterPlayerController* OwnerController = Cast<AShooterPlayerController>(OwnerCharacter->Controller);
        if (OwnerController)
        {
			if (OwnerCharacter->HasAuthority() && !bUseServerSideRewind)
			{
				const float DamageToCause = Hit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage;
				UGameplayStatics::ApplyDamage(OtherActor, DamageToCause, OwnerController, this, UDamageType::StaticClass());
				Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
				return;
			}
			AShooterCharacter* HitCharacter = Cast<AShooterCharacter>(OtherActor);
			if (bUseServerSideRewind && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled() && HitCharacter)
			{
				OwnerCharacter->GetLagCompensation()->ProjectileServerScoreRequest(
					HitCharacter,
					TraceStart,
					InitialVelocity,
					OwnerController->GetServerTime() - OwnerController->SingleTripTime
				);
			}
        }
    }

    Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
