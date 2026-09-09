#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstraeonPlanetGravityComponent.generated.h"

class ACharacter;
class AAstraeonPlanetGravityHarness;

// Gravedad radial para un Character. Spike mínimo de la Fase 0 (ADR 0004): pone la dirección de
// gravedad hacia el centro del cuerpo y orienta la cápsula al arriba local.
//
// Alcance deliberado. Esto **no** es la locomoción planetaria completa: salto, caída, cambio de
// cuerpo, física de objetos, IA y el marco de cámara son trabajo de la Fase 1. Lo que este
// componente tiene que demostrar es que el personaje se sostiene de pie en cualquier orientación,
// que es la puerta de salida de la Fase 0.
//
// Por qué el motor no basta: UE 5.7.4 expone `SetGravityDirection`, que resuelve hacia dónde cae,
// pero **no** alinea la cápsula ni la malla. Esa parte es de este componente, y es el riesgo real
// que el plan de ejecución §7 pide vigilar.
//
// Está inactivo salvo que exista un harness en el mundo, así que puede vivir en el personaje sin
// alterar el recorrido plano actual.
UCLASS(ClassGroup = (Astraeon), meta = (BlueprintSpawnableComponent))
class ASTRAEON_API UAstraeonPlanetGravityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAstraeonPlanetGravityComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Grados por segundo a los que la cápsula persigue el arriba local. Un valor alto orienta al
	// instante y da tirones al cruzar terreno; uno bajo deja al personaje inclinado en pendientes
	// rápidas. Es el parámetro que la Fase 1 tendrá que calibrar contra la cámara.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astraeon|Planet")
	float AlignDegreesPerSecond = 360.0f;

	UFUNCTION(BlueprintPure, Category = "Astraeon|Planet")
	bool IsPlanetGravityActive() const { return bActive; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Planet")
	FVector GetUpVector() const;

	UFUNCTION(BlueprintPure, Category = "Astraeon|Planet")
	FVector GetGravityDirection() const;

	// Altura sobre la superficie de referencia del cuerpo. Negativa por dentro.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Planet")
	double GetAltitudeCm() const;

	UFUNCTION(BlueprintPure, Category = "Astraeon|Planet")
	FVector GetPlanetCenterCm() const { return PlanetCenterCm; }

	// Fija el cuerpo a mano en vez de descubrirlo. Lo usan las pruebas y, más adelante, el cambio
	// de cuerpo activo.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Planet")
	void SetPlanetBody(const FVector& CenterCm, double RadiusCm, float SurfaceGravityMS2);

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Planet")
	void ClearPlanetBody();

private:
	// Busca el harness del mundo y se activa si lo encuentra. Sin harness el componente no toca
	// nada: es lo que permite que el personaje lo lleve siempre montado.
	bool TryBindHarness();

	void ApplyGravityDirection();
	void AlignOwnerToUp(float DeltaSeconds);

	ACharacter* GetOwnerCharacter() const;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Planet")
	bool bActive = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Planet")
	FVector PlanetCenterCm = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Planet")
	double PlanetRadiusCm = 0.0;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Planet")
	float PlanetSurfaceGravityMS2 = 9.81f;

	// La gravedad sólo se reescribe cuando la dirección cambia de verdad. Llamar a
	// SetGravityDirection cada frame con el mismo valor recalcula cuaterniones cacheados del
	// movimiento sin necesidad.
	FVector LastAppliedGravityDirection = FVector::ZeroVector;
};
