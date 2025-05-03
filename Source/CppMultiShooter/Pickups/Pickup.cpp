// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/SphereComponent.h"
#include "CppMultiShooter/Weapon/WeaponTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"

APickup::APickup()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
    OverlapSphere->SetupAttachment(RootComponent);
    OverlapSphere->SetSphereRadius(150.f);
    OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    OverlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    OverlapSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);    

    PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    PickupMesh->SetupAttachment(OverlapSphere);
    PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PickupMesh->SetRenderCustomDepth(true);
    PickupMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_PURPLE);

    OverlapSphere->AddLocalOffset(FVector(0.f, 0.f, 85.f));
    PickupMesh->SetRelativeScale3D(FVector(3.f, 3.f, 3.f));

    PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
    PickupEffectComponent->SetupAttachment(RootComponent);
}

void APickup::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        GetWorldTimerManager().SetTimer(
            BindOverlapTimer,
            this,
            &APickup::BindOverlapTimerFinished,
            BindOverlapTime
        );
    }
}

void APickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
    if (ShooterCharacter)
    {
        OnOverlap(ShooterCharacter);
    }
}

void APickup::OnOverlap(AShooterCharacter* ShooterCharacter)
{
}

void APickup::BindOverlapTimerFinished()
{    
    GetWorldTimerManager().ClearTimer(BindOverlapTimer);
    OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &APickup::OnSphereOverlap);    

    if (HasAuthority())
    {
        TArray<AActor*> OverlappingActors;
        GetOverlappingActors(OverlappingActors, AShooterCharacter::StaticClass());           

        AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(GetClosestActor(OverlappingActors));
        if (ShooterCharacter)
        {
            OnOverlap(ShooterCharacter);
        }
    }
}

AActor* APickup::GetClosestActor(const TArray<AActor*> PlayersToCheck) const
{
    AActor* ClosestPlayer = nullptr;
    float minDist = OverlapSphere->GetScaledSphereRadius();

    for (AActor* PlayerToCheck : PlayersToCheck)
    {
        if (PlayerToCheck)
        {
            const float checkDist = (GetActorLocation() - PlayerToCheck->GetActorLocation()).Length();
            if (checkDist < minDist)
            {
                ClosestPlayer = PlayerToCheck;
                minDist = checkDist;
            }
        }
    }

    return ClosestPlayer;
}

void APickup::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (OverlapSphere)
    {
        OverlapSphere->AddWorldRotation(FRotator(0.f, BaseTurnRate * DeltaTime, 0.f));
    }
}

void APickup::Destroyed()
{
    Super::Destroyed();
    OverlapSphere->OnComponentBeginOverlap.RemoveDynamic(this, &APickup::APickup::OnSphereOverlap); // 함수 중복 호출 방지

    if (PickupSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            PickupSound,
            GetActorLocation()
        );
    }
    if (PickupEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            PickupEffect,
            GetActorLocation(),
            GetActorRotation()
        );
    }
}