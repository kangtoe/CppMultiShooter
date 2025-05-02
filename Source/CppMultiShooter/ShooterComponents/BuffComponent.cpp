// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "CppMultiShooter/Character/ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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
    ShieldRampUp(DeltaTime);
}

#pragma region Heal & Shield
void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
    bHealing = true;
    HealingRate = HealAmount / HealingTime;
    AmountToHeal += HealAmount;
}

void UBuffComponent::ReplenishShield(float ShieldAmount, float ReplenishTime)
{
    bReplenishingShield = true;
    ShieldReplenishRate = ShieldAmount / ReplenishTime;
    ShieldReplenishAmount += ShieldAmount;
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
void UBuffComponent::ShieldRampUp(float DeltaTime)
{
    if (!bReplenishingShield || Character == nullptr || Character->IsElimmed()) return;

    float replenishThisFrame = ShieldReplenishRate * DeltaTime;
    replenishThisFrame = FMath::Clamp(replenishThisFrame, 0, ShieldReplenishAmount);
    ShieldReplenishAmount -= replenishThisFrame;

    Character->SetShield(FMath::Clamp(Character->GetShield() + replenishThisFrame, 0.f, Character->GetMaxShield()));

    if (ShieldReplenishAmount <= 0.f || Character->GetShield() >= Character->GetMaxShield())
    {
        bReplenishingShield = false;
        ShieldReplenishAmount = 0.f;
        return;
    }
}
#pragma endregion

#pragma region speed
void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
    InitialBaseSpeed = BaseSpeed;
    InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime)
{
    if (Character == nullptr) return;

    Character->GetWorldTimerManager().SetTimer(
        SpeedBuffTimer,
        this,
        &UBuffComponent::ResetSpeeds,
        BuffTime
    );

    if (Character->GetCharacterMovement())
    {
        Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
        Character->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
    }
    MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed);
}

void UBuffComponent::ResetSpeeds()
{
    if (Character == nullptr || Character->GetCharacterMovement() == nullptr) return;

    Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
    Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;
    MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
    Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
    Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}
#pragma endregion
