// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CPPMULTISHOOTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UBuffComponent();
    friend class AShooterCharacter;
    void Heal(float HealAmount, float HealingTime);
protected:
    virtual void BeginPlay() override;
    void HealRampUp(float DeltaTime);
private:
    UPROPERTY()
    class AShooterCharacter* Character;

    bool bHealing = false;
    float HealingRate = 0;
    float AmountToHeal = 0.f;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
		
};
