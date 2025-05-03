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
    virtual void OnOverlap(class AShooterCharacter* ShooterCharacter) override;
private:

    UPROPERTY(EditAnywhere)
    float HealAmount = 50.f;

    UPROPERTY(EditAnywhere)
    float HealingTime = 0.5f;
};
