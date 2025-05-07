// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Projectile.generated.h"

UCLASS()
class CPPMULTISHOOTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:
    AProjectile();
    virtual void Tick(float DeltaTime) override;
    virtual void Destroyed() override;

protected:
    virtual void BeginPlay() override;
    void StartDestroyTimer();
    void DestroyTimerFinished();
    void SpawnTrailSystem();
    void ExplodeDamage();

    UFUNCTION()
    virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
    UFUNCTION(NetMulticast, Reliable)
    void MultiCast_OnHit(bool bCharacterHit);

    UPROPERTY(EditAnywhere)
    float Damage = 20.f;

    UPROPERTY(EditAnywhere)
    class UPrimitiveComponent* CollisionBox;

    UPROPERTY(EditAnywhere)
    class UNiagaraSystem* TrailSystem;

    UPROPERTY()
    class UNiagaraComponent* TrailSystemComponent;

    UPROPERTY(VisibleAnywhere)
    class UProjectileMovementComponent* ProjectileMovementComponent;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* ProjectileMesh;

    UPROPERTY(EditAnywhere)
    float DamageInnerRadius = 200.f;

    UPROPERTY(EditAnywhere)
    float DamageOuterRadius = 500.f;

    UPROPERTY(EditAnywhere)
    UParticleSystem* ImpactObstacleParticles;
    UPROPERTY(EditAnywhere)
    UParticleSystem* ImpactCharacterParticles;
    UParticleSystem* ImpactParticles;
    UPROPERTY(EditAnywhere)
    class USoundCue* ImpactSound;

    FTimerHandle DestroyTimer;

    UPROPERTY(EditAnywhere)
    float DestroyTime = 3.f;

private:   
    UPROPERTY(EditAnywhere)
    class UParticleSystem* Tracer;

    UPROPERTY()
    class UParticleSystemComponent* TracerComponent;

public:
    FORCEINLINE UPrimitiveComponent* GetCollisionComponent() { return CollisionBox; }
    FORCEINLINE UPrimitiveComponent* SetDamage(float Damage) { return; }
};
