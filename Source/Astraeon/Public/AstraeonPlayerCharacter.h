#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstraeonPlayerCharacter.generated.h"

class UAstraeonSuitComponent;
class UCameraComponent;

UCLASS()
class ASTRAEON_API AAstraeonPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAstraeonPlayerCharacter();

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Survival")
	TObjectPtr<UAstraeonSuitComponent> SuitComponent;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void StartJump();
	void StopJump();
	void ScanEnvironment();
	void Interact();
	void CraftSignalResonator();
};
