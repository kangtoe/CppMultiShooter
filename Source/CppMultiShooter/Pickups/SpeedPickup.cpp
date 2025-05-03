// Fill out your copyright notice in the Description page of Project Settings.


#include "SpeedPickup.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "CppMultiShooter/ShooterComponents/BuffComponent.h"

void ASpeedPickup::OnOverlap(AShooterCharacter* ShooterCharacter)
{
    if (ShooterCharacter)
    {
        UBuffComponent* Buff = ShooterCharacter->GetBuff();
        if (Buff)
        {
            Buff->BuffSpeed(BaseSpeedBuff, CrouchSpeedBuff, SpeedBuffTime);
        }
    }

    Destroy();
}
