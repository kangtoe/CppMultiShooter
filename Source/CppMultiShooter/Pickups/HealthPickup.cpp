// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "CppMultiShooter/ShooterComponents/BuffComponent.h"

AHealthPickup::AHealthPickup()
{
    bReplicates = true;
}

void AHealthPickup::OnOverlap(AShooterCharacter* ShooterCharacter)
{
    if (ShooterCharacter)
    {
        UBuffComponent* Buff = ShooterCharacter->GetBuff();
        if (Buff)
        {
            Buff->Heal(HealAmount, HealingTime);
        }
    }

    Destroy();
}

