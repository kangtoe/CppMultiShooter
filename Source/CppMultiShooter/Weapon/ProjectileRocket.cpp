// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "NiagaraSystemInstance.h"

AProjectileRocket::AProjectileRocket()
{
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
    ProjectileMesh->SetupAttachment(RootComponent);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
        CollisionBox->IgnoreActorWhenMoving(Owner, true);               
    }

    SpawnTrailSystem();

    if (ProjectileLoop && LoopingSoundAttenuation)
    {
        ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
            ProjectileLoop,
            GetRootComponent(),
            FName(),
            GetActorLocation(),
            EAttachLocation::KeepWorldPosition,
            false,
            1.f,
            1.f,
            0.f,
            LoopingSoundAttenuation,
            (USoundConcurrency*)nullptr,
            false
        );
    }

    if (bExplodAsTimer) StartExplodeTimer();
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if(!bExplodAsTimer) Explode();
}

void AProjectileRocket::Destroyed()
{
    // override to do nothing
}

void AProjectileRocket::Explode()
{
    ExplodeDamage();

    // 사운드/매시 비지블/충돌처리/트레일 파티클 생성 비활성화
    if (ImpactParticles)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
    }
    if (ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
    }
    if (ProjectileMesh)
    {
        ProjectileMesh->SetVisibility(false);
    }
    if (CollisionBox)
    {
        CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
    {
        TrailSystemComponent->GetSystemInstance()->Deactivate();
    }
    if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
    {
        ProjectileLoopComponent->Stop();
    }

    // 기존 적중 처리와는 다르게, 일정 시간 후 삭제 -> 기존 생성된 파티클/사운드 등이 충분히 사라지도록 유예
    //Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
    StartDestroyTimer();
}

void AProjectileRocket::StartExplodeTimer()
{
    GetWorldTimerManager().SetTimer(
        ExplodeTimer,
        this,
        &AProjectileRocket::ExplodeTimerFinished,
        ExplodeTime
    );
}

void AProjectileRocket::ExplodeTimerFinished()
{
    Explode();
}