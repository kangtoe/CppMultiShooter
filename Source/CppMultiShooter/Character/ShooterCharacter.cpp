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
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AShooterCharacter::AShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

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

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
}

void AShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AShooterCharacter, OverlappingWeapon, COND_OwnerOnly);
	//DOREPLIFETIME(AShooterCharacter, Health);
	//DOREPLIFETIME(AShooterCharacter, Shield);
	//DOREPLIFETIME(AShooterCharacter, bDisableGameplay);
}

void AShooterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
	/*if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(
			GetCharacterMovement()->MaxWalkSpeed,
			GetCharacterMovement()->MaxWalkSpeedCrouched
		);
		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}
	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<ABlasterPlayerController>(Controller);
		}
	}*/
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
	}
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 캐릭터가 컨트롤러에 의해 조종되고 있는지 확인
	if (Controller)
	{
		UE_LOG(LogTemp, Display, TEXT("%s is possessed by %s"), *GetName(), *Controller->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s is not possessed"), *GetName());
	}

	// 플레이어 컨트롤러 확인
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		UE_LOG(LogTemp, Display, TEXT("Player Controller is %s"), *PlayerController->GetName());

		// 입력 서브시스템을 가져와 입력 매핑 컨텍스트 추가
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (InputMapping)
			{
				Subsystem->AddMappingContext(InputMapping, 0);
				UE_LOG(LogTemp, Display, TEXT("Input Mapping Context added"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("InputMapping is null!"));
			}
		}
	}	
}

void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
}

#pragma region 캐릭터 입력 처리
void AShooterCharacter::OnInputMove(const FInputActionInstance& Instance)
{
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
	FVector2D LookDirection = Instance.GetValue().Get<FVector2D>();
	AddControllerYawInput(LookDirection.X); // 좌우 회전
	AddControllerPitchInput(LookDirection.Y); // 상하 회전
}

// 점프 입력 처리 함수
void AShooterCharacter::OnInputJump(const FInputActionInstance& Instance)
{
	Super::Jump();
	UE_LOG(LogTemp, Display, TEXT("JumpAction")); // 점프 로그 출력
}

void AShooterCharacter::OnInputEquip(const FInputActionInstance& Instance)
{	
	UE_LOG(LogTemp, Display, TEXT("EquipAction"));

	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerOnInputEquip();
		}
	}

	//if (bDisableGameplay) return;
	//if (Combat)
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

void AShooterCharacter::OnInputCrouch(const FInputActionInstance& Instance)
{
	//if (Combat && Combat->bHoldingTheFlag) return;
	//if (bDisableGameplay) return;
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
	bool Aimed = Instance.GetValue().Get<bool>();

	if (Combat && Aimed)
	{
		Combat->SetAiming(true);
	}
	else
	{
		Combat->SetAiming(false);
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
		bUseControllerRotationYaw = false;

		// AO_Yaw sloution2 (for FPS)
		//float const Yaw = GetBaseAimRotation().Yaw;
		//FVector2D InRange(0.f, 360.f);
		//FVector2D OutRange(10.f, 20.f);
		//AO_Yaw = FMath::GetMappedRangeValueClamped(InRange, OutRange, Yaw);
	}
	if (Speed > 0.f || bIsInAir) // running, or jumping
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
	}
	
	// GetNormalized 사용 이유: AO_Pitch > 90인 경우, 언리얼 내부 멀티플레이 동기화 로직이 값을 압축/해제하는 과정에서 바꿔버림
	AO_Pitch = GetBaseAimRotation().GetNormalized().Pitch;	
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