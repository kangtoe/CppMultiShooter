#include "ProjectileRocketBounce.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

AProjectileRocketBounce::AProjectileRocketBounce()
{			
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->bShouldBounce = true;

	PrimaryActorTick.bCanEverTick = true;		
}

void AProjectileRocketBounce::BeginPlay()
{
	Super::BeginPlay();
	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileRocketBounce::OnBounce);
}

void AProjectileRocketBounce::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			BounceSound,
			GetActorLocation()
		);
	}

	FRotator BounceRotation = ImpactVelocity.Rotation();
	BounceRotation += FRotator(
		FMath::FRandRange(-45.0f, 45.0f), // Pitch
		FMath::FRandRange(-45.0f, 45.0f), // Yaw
		FMath::FRandRange(-45.0f, 45.0f)  // Roll
	);

	//SetActorRotation(BounceRotation);
	ProjectileMesh->SetRelativeRotation(BounceRotation);	
}