// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "CppMultiShooter/ShooterComponents/CombatComponent.h"

void AAmmoPickup::OnOverlap(AShooterCharacter* ShooterCharacter)
{
    if (ShooterCharacter)
    {
        UCombatComponent* Combat = ShooterCharacter->GetCombat();
        if (Combat)
        {
            Combat->PickupAmmo(WeaponType, AmmoAmount);
        }
    }
    Destroy();
}
