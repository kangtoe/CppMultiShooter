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
	void SwapWeapons();
	void ReloadEmptyWeapon();
	void Reload();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	void SetFiring(bool bIsFire);

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished(); // call in montage?

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();	

	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void FinishSwap();
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void FinishSwapAttachWeapons();

protected:
	virtual void BeginPlay() override;
	
	void SetAiming(bool bIsAiming);
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();
	UFUNCTION()
	void OnRep_SecondaryWeapon();
	
	void Fire();
	void LocalFire(const TArray<FVector_NetQuantize>& HitTargets);
	UFUNCTION(Server, Reliable)
	void ServerFire(const TArray<FVector_NetQuantize>& HitTargets);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const TArray<FVector_NetQuantize>& HitTargets);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	void DropEquippedWeapon();
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);	
	void AttachActorToBackpack(AActor* ActorToAttach);	
	//void AttachFlagToLeftHand(AWeapon* Flag);
	
	void UpdateCarriedAmmo();

	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);	
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

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
	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_Aiming)
	bool bAiming;
	UFUNCTION()
	void OnRep_Aiming();
	bool bIsAimingLocal = false;	

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
		
	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 4;
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Grenades)
	int32 Grenades = 1;	
	void UpdateHUDGrenades();
	UFUNCTION()
	void OnRep_Grenades();

public:
	FORCEINLINE int32 GetGrenades() const { return Grenades; }
};
