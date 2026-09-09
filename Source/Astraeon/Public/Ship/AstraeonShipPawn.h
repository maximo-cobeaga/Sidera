#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AstraeonShipPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

// Ítaca en vuelo. La nave y la estancia son el mismo objeto de ficción: mientras esto
// existe, el interior está oculto y se reubica donde el jugador aterrice, así que volar
// significa mudar la base y no sólo mirar el terreno desde arriba.
UCLASS()
class ASTRAEON_API AAstraeonShipPawn : public APawn
{
	GENERATED_BODY()

public:
	AAstraeonShipPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Altura sobre el suelo trazado bajo la nave, en cm. Negativa si no hay suelo debajo.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Ship")
	float GetAltitudeCm() const;

	UFUNCTION(BlueprintPure, Category = "Astraeon|Ship")
	float GetSpeedKmH() const;

	// El techo existe porque todavía no hay ningún destino fuera de esta región: salir de
	// la atmósfera no llevaría a ninguna parte.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Ship")
	bool IsAtAltitudeCeiling() const;

	UFUNCTION(BlueprintPure, Category = "Astraeon|Ship")
	bool CanLandHere() const;

	static float GetAltitudeCeilingCm();

private:
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Ship")
	TObjectPtr<UStaticMeshComponent> HullMesh;

	// Motores, patines y antena. Se guardan para que los VFX de empuje y polvo, cuando
	// existan, tengan de dónde colgarse sin volver a buscarlos por nombre.
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Ship")
	TArray<TObjectPtr<UStaticMeshComponent>> HullModules;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Ship")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Ship")
	TObjectPtr<UCameraComponent> ChaseCamera;

	FVector VelocityCms = FVector::ZeroVector;
	float ThrottleInput = 0.0f;
	float StrafeInput = 0.0f;
	float LiftInput = 0.0f;
	bool bBoosting = false;

	void ApplyThrottle(float Value);
	void ApplyStrafe(float Value);
	void ApplyLift(float Value);
	void StartBoost();
	void StopBoost();
	void RequestLanding();

	bool TraceGroundZ(float& OutGroundZ) const;
};
