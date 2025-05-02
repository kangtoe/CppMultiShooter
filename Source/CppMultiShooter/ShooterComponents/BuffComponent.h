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
protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    class AShooterCharacter* Character;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
		
};
