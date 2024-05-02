// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPSCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Materials/Material.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"

ATPSCharacter::ATPSCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create a decal in the world to show the cursor's location
	CursorToWorld = CreateDefaultSubobject<UDecalComponent>("CursorToWorld");
	CursorToWorld->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UMaterial> DecalMaterialAsset(TEXT("Material'/Game/Blueprint/Character/M_Cursor_Decal.M_Cursor_Decal'")); // /Game/TopDownCPP/Blueprints/M_Cursor_Decal
	if (DecalMaterialAsset.Succeeded())
	{
		CursorToWorld->SetDecalMaterial(DecalMaterialAsset.Object);
	}
	CursorToWorld->DecalSize = FVector(16.0f, 32.0f, 32.0f);
	CursorToWorld->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f).Quaternion());

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ATPSCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CursorToWorld != nullptr)
	{
		if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
		{
			if (UWorld* World = GetWorld())
			{
				FHitResult HitResult;
				FCollisionQueryParams Params(NAME_None, FCollisionQueryParams::GetUnknownStatId());
				FVector StartLocation = TopDownCameraComponent->GetComponentLocation();
				FVector EndLocation = TopDownCameraComponent->GetComponentRotation().Vector() * 2000.0f;
				Params.AddIgnoredActor(this);
				World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params);
				FQuat SurfaceRotation = HitResult.ImpactNormal.ToOrientationRotator().Quaternion();
				CursorToWorld->SetWorldLocationAndRotation(HitResult.Location, SurfaceRotation);
			}
		}
		else if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FHitResult TraceHitResult;
			PC->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
			FVector CursorFV = TraceHitResult.ImpactNormal;
			FRotator CursorR = CursorFV.Rotation();
			CursorToWorld->SetWorldLocation(TraceHitResult.Location);
			CursorToWorld->SetWorldRotation(CursorR);
		}
	}

	MovementTick(DeltaSeconds);
}

void ATPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ATPSCharacter::InputAxisX);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ATPSCharacter::InputAxisY);
	PlayerInputComponent->BindAxis(TEXT("MouseWeel"), this, &ATPSCharacter::SetNewArmLength);

}

void ATPSCharacter::SetNewArmLength(float Value)
{
	//UE_LOG(LogTemp, Warning, TEXT("The float value is: %f"), Value);

	if ((Value < 0 && GetCameraBoom()->TargetArmLength > MinLengthSpringArm) ||
		(Value > 0 && GetCameraBoom()->TargetArmLength < MaxLengthSpringArm)) {

		GetCameraBoom()->TargetArmLength+= Value;
	}
}

bool ATPSCharacter::CanRun()
{
	CalculateAngleBetweenVectors();
	return (Stamina > 0 && abs(AngleBetwenVectors) < 15); // дополнительная проверка отклонения направления движения и взгляда
}

bool ATPSCharacter::CanStartRun()
{
	CalculateAngleBetweenVectors();
	return (Stamina > 100 && abs(AngleBetwenVectors) < 15);  // дополнительная проверка отклонения направления движения и взгляда
}

void ATPSCharacter::ChangeStamina(float Value)
{
	Stamina += Value;
}

void ATPSCharacter::CalculateAngleBetweenVectors()
{
	// Проверка угла между направлением движения и направлением взгляда
	FVector LookVector = GetActorForwardVector();	// направление взгляда (сразу единичный вектора)
	FVector RunVector = GetVelocity();				// направление движения
	RunVector.Normalize();							// нормализуем вектор (получаем единичный вектор)


	// Рассет угла 
	AngleBetwenVectors = acos(Dot3(LookVector, RunVector)) * 180 / PI;
	UE_LOG(LogTemp, Warning, TEXT("Angle between Look and Move vector: %f"), AngleBetwenVectors);
}

void ATPSCharacter::InputAxisX(float Value) {
	AxisX = Value;
}

void ATPSCharacter::InputAxisY(float Value) {
	AxisY = Value;
}

void ATPSCharacter::MovementTick(float DeltaTime)
{
	// Movement
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), AxisX);
	AddMovementInput(FVector(0.0f, 1.0f, 0.0f), AxisY);

	// Rotation
	APlayerController* MyController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (MyController)
	{
		FHitResult ResultHit;
		MyController->GetHitResultUnderCursorByChannel(TraceTypeQuery6, false, ResultHit);

		FRotator NewRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ResultHit.Location);

		NewRotation.Pitch = 0.0f;
		NewRotation.Roll = 0.0f;

		SetActorRotation(NewRotation);		
	}

	// Check Run state
	if (CurrentMovementState == EMovementState::Run_state) {
		if (CanRun()) {
			ChangeStamina(-1.0f);
			//UE_LOG(LogTemp, Warning, TEXT("The current stamina value : %f"), Stamina);
		}
		else {
			ChangeMovementState(EMovementState::Walk_state);
		}
	}
	else if (Stamina < 1000) {
		ChangeStamina(1.0f);
		//UE_LOG(LogTemp, Warning, TEXT("The current stamina value : %f"), Stamina);
	}
}

void ATPSCharacter::CharacterUpdate()
{
	float NewSpeed = 600.0f;

	switch (CurrentMovementState)
	{
	case EMovementState::Aim_state:
		NewSpeed = MovementInfo.AimSpeed;
		break;
	case EMovementState::Walk_state:
		NewSpeed = MovementInfo.WalkSpeed;
		break;
	case EMovementState::Run_state:
		if (CanStartRun()) {
			NewSpeed = MovementInfo.RunSpeed;
		}
		break;
	}

	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;

}

void ATPSCharacter::ChangeMovementState(EMovementState NewMovementState)
{
	CurrentMovementState = NewMovementState;
	CharacterUpdate();
}
	
