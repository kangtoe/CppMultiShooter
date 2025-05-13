	// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputMappingContext.h"
#include "CppMultiShooter/CustomTypes/TurningInPlace.h"
#include "CppMultiShooter/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "CppMultiShooter/CustomTypes/CombatState.h"

#include "ShooterCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

USTRUCT(BlueprintType)
struct FPhysAssetInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FName BoneName;
	UPROPERTY()
	float HalfHeight;
	UPROPERTY()
	float radius;
	UPROPERTY()
	FTransform BoneWorldTransfrom;
};

UCLASS()
class CPPMULTISHOOTER_API AShooterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:	
	AShooterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayThrowMontage();
	void PlaySwapMontage();
	
	void Elim(bool bPlayerLeftGame);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeftGame);
	virtual void Destroyed() override;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	// IMC & AIs
	UPROPERTY(EditAnywhere, Category = Input)
	UInputMappingContext* InputMapping;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* LookAction;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* JumpAction;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* EquipAction;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* CrouchAction;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* AimAction;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* FireAction;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* ReloadAction;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* ThrowAction;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* SwapAction;

	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();
	void UpdateHUDGrenade();

	void SpawnDefaultWeapon();
	//bool bFinishedSwapping = false;

	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();
	FOnLeftGame OnLeftGame;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void OnInputMove(const FInputActionInstance& Instance);
	void OnInputLook(const FInputActionInstance& Instance);
	void OnInputJump(const FInputActionInstance& Instance);
	void OnInputEquip(const FInputActionInstance& Instance);
	void OnInputCrouch(const FInputActionInstance& Instance);
	void OnInputAim(const FInputActionInstance& Instance);
	void OnInputFire(const FInputActionInstance& Instance);
	void OnInputReload(const FInputActionInstance& Instance);
	void OnInputThrow(const FInputActionInstance& Instance);	
	void OnInputSwap(const FInputActionInstance& Instance);

	void AimOffset(float DeltaTime);

	void PlayHitReactMontage();	
	void PlayElimMontage();	

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	
	void RotateInPlace(float DeltaTime);

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	/**
	* Blaster components
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;
	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;
	UPROPERTY(VisibleAnywhere)
	class ULagCompensationComponent* LagCompensation;

	UFUNCTION(Server, Reliable)
	void ServerOnInputEquip();
	UFUNCTION(Server, Reliable)
	void ServerOnInputSwap();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	/**
	* Animation montages
	*/
	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage;
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ThrowMontage;
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* SwapMontage;

	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 175.f;

	/**
	* Player health
	*/
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;
	UFUNCTION()
	void OnRep_Health(float LastHealth);

	/**
	* Player shield
	*/
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_Shield, VisibleAnywhere, Category = "Player Stats")
	float Shield = 0.f;
	UFUNCTION()
	void OnRep_Shield(float LastShield);

	UPROPERTY()
	class AShooterPlayerController* ShooterPlayerController;

	bool bElimmed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	void ElimTimerFinished();

	bool bLeftGame = false;

	/**
	* Dissolve effect
	*/
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;
	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();	
	UPROPERTY(VisibleAnywhere, Category = Elim) // Dynamic instance that we can change at runtime (by code)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;	
	UPROPERTY(EditAnywhere, Category = Elim) // Material instance set on the Blueprint, used with the dynamic material instance
	UMaterialInstance* DissolveMaterialInstance;

	/**
	* Elim bot
	*/
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;
	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;
	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	UPROPERTY()
	class AShooterPlayerState* ShooterPlayerState;

	/**
	* Grenade
	*/
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	/**
	* Default weapon
	*/
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

	/**
	* Hit Boxes (for server rewind)
	*/
	UPROPERTY()
	TMap<FName, class UBoxComponent*> HitCollisionBoxes;
	void AddCollisionBox(const FName& BoneName, const FVector& RelativeLocation, const FRotator& RelativeRotation, const FVector& BoxExtent);
	void CreateCollisionBoxes();

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; UpdateHUDHealth(); }
	
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; UpdateHUDShield(); }
	
	ECombatState GetCombatState() const;
	
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	
	FORCEINLINE auto& GetHitCollisionBoxes() const { return HitCollisionBoxes; }
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
};
