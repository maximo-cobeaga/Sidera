#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstraeonItacaInterior.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;

/** Reusable metric Q1 room. Same assembly in the art test map and the expedition. */
UCLASS()
class ASTRAEON_API AAstraeonItacaInterior : public AActor
{
	GENERATED_BODY()
public:
	AAstraeonItacaInterior();
	virtual void Tick(float DeltaSeconds) override;
	static bool IsInsideFootprint(const FVector2D& XY, float MarginCm = 0);

	// Grosor de la cubierta: el piso de la estancia ocupa de -12 a 0 en el espacio del
	// actor. Ítaca se posa este tanto por encima del terreno para que su cubierta se apoye
	// encima y no quede a la misma cota, que producía z-fighting y doble colisión.
	static float GetDeckThicknessCm();

	// La escotilla dejó de ser un hueco abierto: la hoja bloquea el paso hasta que el
	// jugador la abre desde el marcador, que es lo que el propietario reportó que faltaba.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Itaca")
	void OpenHatch();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Itaca")
	bool IsHatchOpen() const { return bHatchOpen; }

	// Ángulo de apertura de la hoja, en grados. Negativo: gira hacia dentro de la estancia.
	static float GetHatchOpenAngleDegrees();

private:
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Itaca")
	TObjectPtr<USceneComponent> HatchHinge;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Itaca")
	TObjectPtr<UStaticMeshComponent> HatchLeaf;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Itaca")
	TObjectPtr<UBoxComponent> HatchLeafCollision;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Itaca")
	bool bHatchOpen = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Itaca")
	float HatchAngleDegrees = 0.0f;
};
