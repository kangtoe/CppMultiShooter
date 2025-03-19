// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "CombatComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CPPMULTISHOOTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class AShooterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;	

	void EquipWeapon(class AWeapon* WeaponToEquip);

protected:
	virtual void BeginPlay() override;
	/*void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);*/

	UFUNCTION()
	void OnRep_EquippedWeapon();

	/*UFUNCTION()
	void OnRep_SecondaryWeapon();*/

	//void DropEquippedWeapon();
	void AttachActorToRightHand(AActor* ActorToAttach);
	//void AttachActorToLeftHand(AActor* ActorToAttach);
	//void AttachFlagToLeftHand(AWeapon* Flag);
	//void AttachActorToBackpack(AActor* ActorToAttach);

	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	//void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

private:
	UPROPERTY()
	class AShooterCharacter* Character;
	//UPROPERTY()
	//class ABlasterPlayerController* Controller;
	//UPROPERTY()
	//class ABlasterHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;
	/*UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;*/
};
