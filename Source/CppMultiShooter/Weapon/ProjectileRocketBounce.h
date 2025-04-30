// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectileRocket.h"
#include "ProjectileRocketBounce.generated.h"

/**
 * 
 */
UCLASS()
class CPPMULTISHOOTER_API AProjectileRocketBounce : public AProjectileRocket
{
	GENERATED_BODY()
public:
	AProjectileRocketBounce();
	//virtual void Destroyed() override;
protected:
	virtual void BeginPlay() override;	

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);
private:

	UPROPERTY(EditAnywhere)
	USoundCue* BounceSound;

	UPROPERTY(EditAnywhere)
	float BounceMinVelocity = 1.0f;
};
