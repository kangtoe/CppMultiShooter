// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "ShieldPickup.generated.h"

/**
 * 
 */
UCLASS()
class CPPMULTISHOOTER_API AShieldPickup : public APickup
{
	GENERATED_BODY()
protected:
    virtual void OnOverlap(class AShooterCharacter* ShooterCharacter) override;
private:

    UPROPERTY(EditAnywhere)
    float ShieldReplenishAmount = 50.f;

    UPROPERTY(EditAnywhere)
    float ShieldReplenishTime = 0.5f;
};
