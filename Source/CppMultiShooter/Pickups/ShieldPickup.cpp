// Fill out your copyright notice in the Description page of Project Settings.


#include "ShieldPickup.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "CppMultiShooter/ShooterComponents/BuffComponent.h"

void AShieldPickup::OnOverlap(AShooterCharacter* ShooterCharacter)
{
    if (ShooterCharacter)
    {
        UBuffComponent* Buff = ShooterCharacter->GetBuff();
        if (Buff)
        {
            Buff->ReplenishShield(ShieldReplenishAmount, ShieldReplenishTime);
        }
    }

    Destroy();
}
