// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CppMultiShooter/HUD/ShooterHUD.h"
#include "CppMultiShooter/Weapon/WeaponTypes.h"
#include "CppMultiShooter/CustomTypes/CombatState.h"

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

	void ReloadEmptyWeapon();
	void Reload();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	void SetFiring(bool bIsFire);

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished(); // call in montage?

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();	

protected:
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	
	void Fire();

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	/*UFUNCTION()
	void OnRep_SecondaryWeapon();*/

	void DropEquippedWeapon();
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);	
	void UpdateCarriedAmmo();	
	//void AttachFlagToLeftHand(AWeapon* Flag);
	//void AttachActorToBackpack(AActor* ActorToAttach);

	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);	
	//void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();
	int32 AmountToReload();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

	void ThrowGrenade();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	void ShowAttachedGrenade(bool bShowGrenade);

private:
	UPROPERTY()
	class AShooterCharacter* Character;
	UPROPERTY()
	class AShooterPlayerController* Controller;
	UPROPERTY()
	class AShooterHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;
	/*UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;*/

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	/**
	* HUD and crosshairs
	*/
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	FVector HitTarget;

	FHUDPackage HUDPackage;

	/**
	* Aiming and FOV
	*/	
	float DefaultFOV; // Field of view when not aiming; set to the camera's base FOV in BeginPlay
	float CurrentFOV;
	//UPROPERTY(EditAnywhere, Category = Combat)
	//float ZoomedFOV = 30.f;	
	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f; 

	void InterpFOV(float DeltaTime);

	/**
	* Automatic fire
	*/
	FTimerHandle FireTimer;
	bool bCanFire = true;

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();

	// Carried ammo for the currently-equipped weapon
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	UPROPERTY(EditAnywhere)
	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();

	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);
};
