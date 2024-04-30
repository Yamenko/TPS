
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../FunctionLibrary/Types.h"
#include "TPSCharacter.generated.h"

UCLASS(Blueprintable)
class ATPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATPSCharacter();

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns CursorToWorld subobject **/
	FORCEINLINE class UDecalComponent* GetCursorToWorld() { return CursorToWorld; }

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	//class USpringArmComponent* SpringArmComopnent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** A decal that projects to the cursor location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UDecalComponent* CursorToWorld;

public:
	float AxisX = 0.0f;
	float AxisY = 0.0f;
	float AxisMouseWeel = 0.0f;
	UFUNCTION()
	void InputAxisX(float Value);

	UFUNCTION()
	void InputAxisY(float Value);

	UFUNCTION()
	void MovementTick(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void CharacterUpdate();
	UFUNCTION(BlueprintCallable)
	void ChangeMovementState(EMovementState NewMovementState);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FCharacterSpeed MovementInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	EMovementState CurrentMovementState = EMovementState::Run_state;

	//----------------------------------------------------------------
	//				SPRING ARM COMPONENT
	//----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MaxLengthSpringArm = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MinLengthSpringArm = 500.0f;

	UFUNCTION()
	void SetNewArmLength(float Value);
};

