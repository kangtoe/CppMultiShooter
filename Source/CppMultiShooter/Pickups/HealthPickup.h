// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "HealthPickup.generated.h"

/**
 * 
 */
UCLASS()
class CPPMULTISHOOTER_API AHealthPickup : public APickup
{
	GENERATED_BODY()
public:
    AHealthPickup();
protected:
    virtual void OnSphereOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );
private:

    UPROPERTY(EditAnywhere)
    float HealAmount = 50.f;

    UPROPERTY(EditAnywhere)
    float HealingTime = 0.5f;
};
