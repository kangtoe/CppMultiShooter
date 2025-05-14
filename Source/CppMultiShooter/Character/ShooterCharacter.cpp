// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "CppMultiShooter/Weapon/Weapon.h"
#include "CppMultiShooter/ShooterComponents/CombatComponent.h"
#include "CppMultiShooter/ShooterComponents/BuffComponent.h"
#include "CppMultiShooter/ShooterComponents/LagCompensationComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ShooterAnimInstance.h"
#include "CppMultiShooter/CppMultiShooter.h"
#include "CppMultiShooter/PlayerController/ShooterPlayerController.h"
#include "CppMultiShooter\GameMode\ShooterGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "CppMultiShooter/PlayerState/ShooterPlayerState.h"
#include "CppMultiShooter/Weapon/WeaponTypes.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "CppMultiShooter/GameState/ShooterGameState.h"


// Sets default values
AShooterCharacter::AShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 캐릭터가 회전하는 방식 설정
	bUseControllerRotationYaw = false; // 컨트롤러의 Yaw 회전을 사용하지 않음
	GetCharacterMovement()->bOrientRotationToMovement = true; // 이동 방향으로 회전하도록 설정

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	// 클라이언트가 (카메라에 안 보여도) 다른 클라이언트의 캐릭터 애니메이션 계산을 항상 수행하도록 함
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	GetCharacterMovement()->RotationRate = FRotator(0.f, 850.f, 0.f);
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);	
}

void AShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AShooterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(AShooterCharacter, Health);
	DOREPLIFETIME(AShooterCharacter, Shield);
	DOREPLIFETIME(AShooterCharacter, bDisableGameplay);
}

void AShooterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(
			GetCharacterMovement()->MaxWalkSpeed,
			GetCharacterMovement()->MaxWalkSpeedCrouched
		);
		//Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}	
	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<AShooterPlayerController>(Controller);
		}
	}	

	CreateCollisionBoxes();
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// EnhancedInputComponent로 캐스팅하여 입력 바인딩 수행
	if (UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{		
		// 입력 바인딩		
		Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AShooterCharacter::OnInputJump);
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AShooterCharacter::OnInputMove);
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AShooterCharacter::OnInputLook);
		Input->BindAction(EquipAction, ETriggerEvent::Triggered, this, &AShooterCharacter::OnInputEquip);
		Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &AShooterCharacter::OnInputCrouch);
		Input->BindAction(EquipAction, ETriggerEvent::Triggered, this, &AShooterCharacter::OnInputEquip);
		Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &AShooterCharacter::OnInputCrouch);
		Input->BindAction(AimAction, ETriggerEvent::Triggered, this, &AShooterCharacter::OnInputAim);
		Input->BindAction(FireAction, ETriggerEvent::Triggered, this, &AShooterCharacter::OnInputFire);
		Input->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &AShooterCharacter::OnInputReload);
		Input->BindAction(ThrowAction, ETriggerEvent::Triggered, this, &AShooterCharacter::OnInputThrow);
		Input->BindAction(SwapAction, ETriggerEvent::Triggered, this, &AShooterCharacter::OnInputSwap);
	}
}

#pragma region Play Montage

void AShooterCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage, 1, EMontagePlayReturnType::MontageLength, 0, false);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AShooterCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage, 1, EMontagePlayReturnType::MontageLength, 0, false);
		FName SectionName;

		switch (Combat->EquippedWeapon->GetWeaponType())
		{
			case EWeaponType::EWT_AssaultRifle:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_RocketLauncher:
				SectionName = FName("RocketLauncher");
				break;
			case EWeaponType::EWT_Pistol:
				SectionName = FName("Pistol");
				break;
			case EWeaponType::EWT_SubmachineGun:
				SectionName = FName("Pistol");
				break;
			case EWeaponType::EWT_Shotgun:
				SectionName = FName("Shotgun");
				break;
			case EWeaponType::EWT_SniperRifle:
				SectionName = FName("SniperRifle");
				break;
			case EWeaponType::EWT_GrenadeLauncher:
				SectionName = FName("GrenadeLauncher");
				break;
		}

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AShooterCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage, 1, EMontagePlayReturnType::MontageLength, 0, false);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AShooterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage, 1, EMontagePlayReturnType::MontageLength, 0, true);
	}
}
void AShooterCharacter::PlayThrowMontage()
{	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowMontage)
	{		
		AnimInstance->Montage_Play(ThrowMontage, 1, EMontagePlayReturnType::MontageLength, 0, false);
	}
}
void AShooterCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SwapMontage)
	{
		AnimInstance->Montage_Play(SwapMontage, 1, EMontagePlayReturnType::MontageLength, 0, false);
	}
}
#pragma endregion

void AShooterCharacter::Elim(bool bPlayerLeftGame)
{
	if (Combat)
	{
		TArray<AWeapon*> Weapons = { Combat->EquippedWeapon, Combat->SecondaryWeapon };
		for (AWeapon* Weapon : Weapons)
		{
			if (Weapon == nullptr) continue;
			if (Weapon->bDestroyOnElim)
			{
				Weapon->Destroy();
			}
			else
			{
				Weapon->Dropped();
			}
		}	
	}

	MulticastElim(bPlayerLeftGame);	
}

void AShooterCharacter::ElimTimerFinished()
{	
	ShooterGameMode = ShooterGameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : ShooterGameMode;

	if (ShooterGameMode && !bLeftGame)
	{
		ShooterGameMode->RequestRespawn(this, Controller);
	}
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}

void AShooterCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;
	if (ShooterPlayerController)
	{
		ShooterPlayerController->SetHUDWeaponAmmo(-1);
	}

	bElimmed = true;
	PlayElimMontage();

	if (DissolveMaterialInstance) // Start dissolve effect
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// Disable character movement
	bDisableGameplay = true;
	GetCharacterMovement()->DisableMovement();
	if (Combat)
	{
		Combat->SetFiring(false);
	}

	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn elim bot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}

	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&AShooterCharacter::ElimTimerFinished,
		ElimDelay
	);

	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
}

void AShooterCharacter::MulticastGainedTheLead_Implementation()
{
	if (CrownSystem == nullptr) return;
	if (!IsValid(CrownComponent))
	{
		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetCapsuleComponent(),
			FName(),
			GetActorLocation() + FVector(0.f, 0.f, 110.f),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
	if (CrownComponent)
	{
		CrownComponent->Activate();
	}
}

void AShooterCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
}

void AShooterCharacter::ServerLeaveGame_Implementation()
{
	ShooterGameMode = ShooterGameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : ShooterGameMode;
	ShooterPlayerState = ShooterPlayerState == nullptr ? GetPlayerState<AShooterPlayerState>() : ShooterPlayerState;
	if (ShooterGameMode && ShooterPlayerState)
	{
		ShooterGameMode->PlayerLeftGame(ShooterPlayerState);
	}
}

void AShooterCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	ShooterGameMode = ShooterGameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : ShooterGameMode;
	bool bMatchNotInProgress = ShooterGameMode && ShooterGameMode->GetMatchState() != MatchState::InProgress;
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void AShooterCharacter::SetTeamColor(ETeam Team)
{
	/*if (GEngine)
	{
		FString f = UEnum::GetValueAsString(Team);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, f);
	}*/

	if (GetMesh() == nullptr || OriginalMaterial == nullptr) return;
	switch (Team)
	{
	case ETeam::ET_NoTeam:
		GetMesh()->SetMaterial(0, OriginalMaterial);
		DissolveMaterialInstance = BlueDissolveMatInst;
		break;
	case ETeam::ET_BlueTeam:
		GetMesh()->SetMaterial(0, BlueMaterial);
		DissolveMaterialInstance = BlueDissolveMatInst;
		break;
	case ETeam::ET_RedTeam:
		GetMesh()->SetMaterial(0, RedMaterial);
		DissolveMaterialInstance = RedDissolveMatInst;
		break;
	}
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();

	UpdateHUDAmmo();
	UpdateHUDHealth();
	UpdateHUDShield();
	UpdateHUDGrenade();

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AShooterCharacter::ReceiveDamage);
	}

	// 플레이어 컨트롤러 확인
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{		
		// 입력 서브시스템을 가져와 입력 매핑 컨텍스트 추가
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (InputMapping)
			{
				Subsystem->AddMappingContext(InputMapping, 0);
			}
		}
	}	
	else if (const ULocalPlayer* Player = (GEngine && GetWorld()) ? GEngine->GetFirstGamePlayer(GetWorld()) : nullptr) // for server (match state의 웜업 기간, 해당 시점에 APlayerController가 없음)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player);
		if (InputMapping)
		{
			Subsystem->AddMappingContext(InputMapping, 0);
		}
	}

	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}		
}

void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);	
	HideCameraIfCharacterClose();	

	AShooterGameState* ShooterGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
	if (ShooterGameState && ShooterGameState->TopScoringPlayers.Contains(ShooterPlayerState))
	{
		MulticastGainedTheLead();
	}	

	ShooterPlayerState = ShooterPlayerState == nullptr ? GetPlayerState<AShooterPlayerState>() : ShooterPlayerState;
	if (ShooterPlayerState)
	{
		SetTeamColor(ShooterPlayerState->GetTeam());
	}	
}

#pragma region 캐릭터 입력 처리
void AShooterCharacter::OnInputMove(const FInputActionInstance& Instance)
{
	if (bDisableGameplay) return;

	// 입력된 이동 방향 값 가져오기
	FVector2D MovementDirection = Instance.GetValue().Get<FVector2D>();

	// 현재 컨트롤러의 Yaw, Roll을 이용하여 방향 벡터 계산
	const FRotator Rotation(0.f, Controller->GetControlRotation().Yaw, Controller->GetControlRotation().Roll);
	const FVector RightDirection(FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y)); // 오른쪽 방향
	const FVector ForwardDirection(FRotationMatrix(Rotation).GetUnitAxis(EAxis::X)); // 전방 방향

	// 입력된 방향으로 이동 적용
	AddMovementInput(RightDirection, MovementDirection.X);
	AddMovementInput(ForwardDirection, MovementDirection.Y);
}

// 마우스 또는 컨트롤러를 사용한 카메라 회전 처리
void AShooterCharacter::OnInputLook(const FInputActionInstance& Instance)
{
	//if (bDisableGameplay) return; // 회전은 허용?

	FVector2D LookDirection = Instance.GetValue().Get<FVector2D>();
	AddControllerYawInput(LookDirection.X); // 좌우 회전
	AddControllerPitchInput(LookDirection.Y); // 상하 회전
}

// 점프 입력 처리 함수
void AShooterCharacter::OnInputJump(const FInputActionInstance& Instance)
{
	if (bDisableGameplay) return;
	if (bIsCrouched)
	{
		UnCrouch();		
	}
	else
	{
		Super::Jump();
	}
}

void AShooterCharacter::OnInputEquip(const FInputActionInstance& Instance)
{	
	if (bDisableGameplay) return;
	ServerOnInputEquip();

	//{
	//	if (Combat->bHoldingTheFlag) return;
	//	if (Combat->CombatState == ECombatState::ECS_Unoccupied) ServerEquipButtonPressed();
	//	bool bSwap = Combat->ShouldSwapWeapons() &&
	//		!HasAuthority() &&
	//		Combat->CombatState == ECombatState::ECS_Unoccupied &&
	//		OverlappingWeapon == nullptr;

	//	if (bSwap)
	//	{
	//		PlaySwapMontage();
	//		Combat->CombatState = ECombatState::ECS_SwappingWeapons;
	//		bFinishedSwapping = false;
	//	}
	//}
}

void AShooterCharacter::ServerOnInputEquip_Implementation()
{
	if (Combat)
	{
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		/*else if (Combat->ShouldSwapWeapons())
		{
			Combat->SwapWeapons();
		}*/
	}
}

void AShooterCharacter::OnInputSwap(const FInputActionInstance& Instance)
{

	/*if (GEngine)
	{
		FColor color = HasAuthority() ? FColor::Red : FColor::Blue;

		GEngine->AddOnScreenDebugMessage(-1, 5.f, color,
			FString::Printf(TEXT("OnInputSwap")));
	}*/

	if (Combat->CombatState == ECombatState::ECS_Unoccupied)
	{
		ServerOnInputSwap();
		if (Combat) Combat->SwapWeapons();
	}
}

void AShooterCharacter::ServerOnInputSwap_Implementation()
{
	/*if (GEngine)
	{
		FColor color = HasAuthority() ? FColor::Red : FColor::Blue;

		GEngine->AddOnScreenDebugMessage(-1, 5.f, color,
			FString::Printf(TEXT("ServerOnInputSwap_Implementation")));
	}*/

	if (Combat) Combat->SwapWeapons();
}

void AShooterCharacter::OnInputCrouch(const FInputActionInstance& Instance)
{	
	if (bDisableGameplay) return;
	//if (Combat && Combat->bHoldingTheFlag) return;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AShooterCharacter::OnInputAim(const FInputActionInstance& Instance)
{
	if (bDisableGameplay) return;
	if (!Combat) return;
	bool Aimed = Instance.GetValue().Get<bool>();	

	if (Aimed)
	{
		Combat->SetAiming(true);
	}
	else
	{
		Combat->SetAiming(false);
	}
}

void AShooterCharacter::OnInputFire(const FInputActionInstance& Instance)
{
	if (bDisableGameplay) return;
	if (!Combat) return;
	bool fire = Instance.GetValue().Get<bool>();

	if (fire)
	{
		Combat->SetFiring(true);
	}
	else
	{
		Combat->SetFiring(false);
	}
}

void AShooterCharacter::OnInputReload(const FInputActionInstance& Instance)
{	
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->Reload();
	}
}

void AShooterCharacter::OnInputThrow(const FInputActionInstance& Instance)
{	
	if (Combat)
	{
		Combat->ThrowGrenade();
	}
}

#pragma endregion

void AShooterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void AShooterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();

	if (!bElimmed && Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void AShooterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{	
	if (bElimmed) return;

	ShooterGameMode = ShooterGameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : ShooterGameMode;
	if (ShooterGameMode == nullptr) return;
	Damage = ShooterGameMode->CalculateDamage(InstigatorController, Controller, Damage);	
	
	float DamageToHealth = Damage;
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else
		{			
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
			Shield = 0.f;
		}
	}

	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();

	if (Health == 0.f)
	{
		ShooterGameMode = ShooterGameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : ShooterGameMode;
		if (ShooterGameMode)
		{
			ShooterPlayerController = ShooterPlayerController == nullptr ? Cast<AShooterPlayerController>(Controller) : ShooterPlayerController;
			AShooterPlayerController* AttackerController = Cast<AShooterPlayerController>(InstigatorController);
			ShooterGameMode->PlayerEliminated(this, ShooterPlayerController, AttackerController);
		}
	}
}

void AShooterCharacter::UpdateHUDHealth()
{	
	ShooterPlayerController = ShooterPlayerController == nullptr ? Cast<AShooterPlayerController>(Controller) : ShooterPlayerController;
	if (ShooterPlayerController)
	{
		ShooterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void AShooterCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();
	if (Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}

void AShooterCharacter::UpdateHUDShield()
{
	ShooterPlayerController = ShooterPlayerController == nullptr ? Cast<AShooterPlayerController>(Controller) : ShooterPlayerController;
	if (ShooterPlayerController)
	{
		ShooterPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void AShooterCharacter::UpdateHUDAmmo()
{
	ShooterPlayerController = ShooterPlayerController == nullptr ? Cast<AShooterPlayerController>(Controller) : ShooterPlayerController;
	if (ShooterPlayerController && Combat)
	{
		if (Combat->EquippedWeapon)
		{						
			ShooterPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
			ShooterPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
		}
		/*else
		{
			ShooterPlayerController->SetHUDCarriedAmmo(-1);
			ShooterPlayerController->SetHUDWeaponAmmo(-1);
		}*/
	}
}

void AShooterCharacter::UpdateHUDGrenade()
{
	ShooterPlayerController = ShooterPlayerController == nullptr ? Cast<AShooterPlayerController>(Controller) : ShooterPlayerController;
	if (ShooterPlayerController && Combat)
	{
		ShooterPlayerController->SetHUDGrenades(GetCombat()->GetGrenades());
	}
	
}

void AShooterCharacter::SpawnDefaultWeapon()
{	
	UWorld* World = GetWorld();
	if (World && !bElimmed && DefaultWeaponClass)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);		
		if (Combat)
		{
			Combat->EquipWeapon(StartingWeapon);
		}
	}
}

void AShooterCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	else
	{
		AimOffset(DeltaTime);
	}
	
}

void AShooterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void AShooterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &AShooterCharacter::UpdateDissolveMaterial); // using delegate, bind callback
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void AShooterCharacter::AddCollisionBox(const FName& BoneName, const FVector& RelativeLocation, const FRotator& RelativeRotation, const FVector& BoxExtent)
{
	if (GetMesh())
    {
        UBoxComponent* BoxComponent = NewObject<UBoxComponent>(this);
        if (BoxComponent)
        {
            BoxComponent->SetupAttachment(GetMesh(), BoneName);
            BoxComponent->RegisterComponent();
            BoxComponent->SetCollisionObjectType(ECC_HitBox);
            BoxComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
            BoxComponent->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
            BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            BoxComponent->SetRelativeLocation(RelativeLocation);
            BoxComponent->SetRelativeRotation(RelativeRotation);
            BoxComponent->SetBoxExtent(BoxExtent);

            // If entry exists, append an index to the name
            uint32 i = 1;
            FName UniqueName = BoneName;
            while (HitCollisionBoxes.Find(UniqueName))
            {
                UniqueName = FName(FString::Printf(TEXT("%s_%d"), *BoneName.ToString(), i));
            }

            HitCollisionBoxes.Add(UniqueName, BoxComponent);
        }
    }
}

void AShooterCharacter::CreateCollisionBoxes()
{
	UPhysicsAsset* PhysicsAsset = GetMesh()->GetPhysicsAsset();
	if (PhysicsAsset)
	{
		for (USkeletalBodySetup* BodySetup : PhysicsAsset->SkeletalBodySetups)
		{
			FName BoneName = BodySetup->BoneName;

			// Capsule elements - Cylinder with spherical top/bottom (sphyinder).
			for (const FKSphylElem& Capsule : BodySetup->AggGeom.SphylElems)
			{
				// Calculate extent (measured from center of box)
				float HalfCylinderLength = Capsule.Length / 2;
				float Radius = Capsule.Radius;
				FVector BoxExtent(Radius, Radius, HalfCylinderLength + Radius);
				AddCollisionBox(BoneName, Capsule.Center, Capsule.Rotation, BoxExtent);
			}

			// Box elements
			for (const FKBoxElem& Box : BodySetup->AggGeom.BoxElems)
			{
				FVector BoxExtent(Box.X, Box.Y, Box.Z);
				AddCollisionBox(BoneName, Box.Center, Box.Rotation, BoxExtent);
			}

			// Sphere elements
			for (const FKSphereElem& Sphere : BodySetup->AggGeom.SphereElems)
			{
				FVector BoxExtent(Sphere.Radius, Sphere.Radius, Sphere.Radius);
				AddCollisionBox(BoneName, Sphere.Center, FRotator::ZeroRotator, BoxExtent);
			}

			// Convex elements
			for (const FKConvexElem& Convex : BodySetup->AggGeom.ConvexElems)
			{
				unimplemented();
			}

			// Tapered Capsule elements
			for (const FKTaperedCapsuleElem& Capsule : BodySetup->AggGeom.TaperedCapsuleElems)
			{
				unimplemented();
			}

			// LevelSet elements
			for (const FKLevelSetElem& LevelSet : BodySetup->AggGeom.LevelSetElems)
			{
				unimplemented();
			}
		}
	}
}

void AShooterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled()) 
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void AShooterCharacter::AimOffset(float DeltaTime)
{	
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // standing still, not jumping
	{

		// AO_Yaw sloution1 (in TPS cource)
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;		

		// AO_Yaw sloution2 (for FPS)
		//float const Yaw = GetBaseAimRotation().Yaw;
		//FVector2D InRange(0.f, 360.f);
		//FVector2D OutRange(10.f, 20.f);
		//AO_Yaw = FMath::GetMappedRangeValueClamped(InRange, OutRange, Yaw);

		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;

		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir) // running, or jumping
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
	
	// GetNormalized 사용 이유: AO_Pitch > 90인 경우, 언리얼 내부 멀티플레이 동기화 로직이 값을 압축/해제하는 과정에서 바꿔버림
	AO_Pitch = GetBaseAimRotation().GetNormalized().Pitch;
}

void AShooterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	// AO_Yaw 보간
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f) // 일정 이하 각도에서는 회전하지 않음
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void AShooterCharacter::HideCameraIfCharacterClose() // 카메라에 캐릭터가 너무 가까우면 숨기기
{
	if (!IsLocallyControlled()) return;

	const bool bCameraTooClose = (FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold;
	GetMesh()->SetVisibility(!bCameraTooClose);
	if (Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = bCameraTooClose;
	}
	if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
	{
		Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = bCameraTooClose;
	}
}

bool AShooterCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool AShooterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AWeapon* AShooterCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector AShooterCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

ECombatState AShooterCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}
