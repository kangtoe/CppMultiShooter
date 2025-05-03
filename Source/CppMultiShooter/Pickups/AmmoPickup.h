// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "CppMultiShooter/Weapon/WeaponTypes.h"
#include "AmmoPickup.generated.h"

/**
 * 
 */
UCLASS()
class CPPMULTISHOOTER_API AAmmoPickup : public APickup
{
    GENERATED_BODY()
protected:
    virtual void OnOverlap(class AShooterCharacter* ShooterCharacter) override;
private:
    UPROPERTY(EditAnywhere)
    int32 AmmoAmount = 30;

    UPROPERTY(EditAnywhere)
    EWeaponType WeaponType;
};
