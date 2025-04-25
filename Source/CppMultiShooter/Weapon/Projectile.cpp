
#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "CppMultiShooter/CppMultiShooter.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

AProjectile::AProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    SetRootComponent(CollisionBox);
    CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
    CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);

    CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);
    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
    ProjectileMovementComponent->bRotationFollowsVelocity = true;
}

void AProjectile::BeginPlay()
{
    Super::BeginPlay();

    // 발사체 궤적 효과
    if (Tracer)
    {
        TracerComponent = UGameplayStatics::SpawnEmitterAttached(
            Tracer,
            CollisionBox,
            FName(),
            GetActorLocation(),
            GetActorRotation(),
            EAttachLocation::KeepWorldPosition
        );
    }

    if (HasAuthority())
    {
        CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
    }    

    // 임시코드 - 기본 적중 파티클 효과 설정
    ImpactParticles = ImpactObstacleParticles ? ImpactObstacleParticles : ImpactCharacterParticles; 
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
    bool bCharacterHit = false;
    if (ShooterCharacter)
    {        
        bCharacterHit = true;        
    }

    // optional challange to show different particles on hit.
    ImpactParticles = bCharacterHit ? ImpactCharacterParticles : ImpactObstacleParticles;
    
    bCharacterHit = true;
    MultiCast_OnHit(bCharacterHit);
    // original code
    // Destroy();
}

void AProjectile::MultiCast_OnHit_Implementation(bool bCharacterHit)
{    
    Destroy();
}

void AProjectile::SpawnTrailSystem()
{
    if (TrailSystem)
    {
        TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
            TrailSystem,
            GetRootComponent(),
            FName(),
            GetActorLocation(),
            GetActorRotation(),
            EAttachLocation::KeepWorldPosition,
            false
        );
    }
}

void AProjectile::ExplodeDamage()
{
    APawn* FiringPawn = GetInstigator();
    if (FiringPawn && HasAuthority())
    {
        AController* FiringController = FiringPawn->GetController();
        if (FiringController)
        {
            UGameplayStatics::ApplyRadialDamageWithFalloff(
                this, // World context object
                Damage, // BaseDamage
                10.f, // MinimumDamage
                GetActorLocation(), // Origin
                DamageInnerRadius, // DamageInnerRadius
                DamageOuterRadius, // DamageOuterRadius
                1.f, // DamageFalloff
                UDamageType::StaticClass(), // DamageTypeClass
                TArray<AActor*>(), // IgnoreActors
                this, // DamageCauser
                FiringController // InstigatorController
            );
        }
    }
}

void AProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AProjectile::StartDestroyTimer()
{
    GetWorldTimerManager().SetTimer(
        DestroyTimer,
        this,
        &AProjectile::DestroyTimerFinished,
        DestroyTime
    );
}

void AProjectile::DestroyTimerFinished()
{
    Destroy();
}

void AProjectile::Destroyed()
{
    Super::Destroyed();

    if (ImpactParticles)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
    }
    if (ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
    }
}

