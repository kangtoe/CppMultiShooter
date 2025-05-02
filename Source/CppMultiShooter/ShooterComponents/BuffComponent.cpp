// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"

UBuffComponent::UBuffComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    HealRampUp(DeltaTime);
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
    bHealing = true;
    HealingRate = HealAmount / HealingTime;
    AmountToHeal += HealAmount;
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
    if (!bHealing || Character == nullptr || Character->IsElimmed()) return;

    float healThisFrame = HealingRate * DeltaTime;
    healThisFrame = FMath::Clamp(healThisFrame, 0, AmountToHeal);
    AmountToHeal -= healThisFrame;      

    Character->SetHealth(FMath::Clamp(Character->GetHealth() + healThisFrame, 0.f, Character->GetMaxHealth()));    
    
    if (AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth())
    {
        bHealing = false;
        AmountToHeal = 0.f;
        return;
    }
}
