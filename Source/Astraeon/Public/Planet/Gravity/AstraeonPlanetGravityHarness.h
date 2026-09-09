#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstraeonPlanetGravityHarness.generated.h"

class USceneComponent;
class UStaticMeshComponent;

// Banco de pruebas del ADR 0004 §1.2. Una esfera escalada con colisión que existe **sólo** para
// validar gravedad radial, orientación del personaje y cámara.
//
// NO es el terreno del juego y no debe evolucionar por subdivisión manual: el planeta jugable lo
// administra el runtime planetario por patches a partir de la Fase 2. Si este actor empieza a
// recibir biomas, props o contenido, algo se está construyendo en el sitio equivocado.
//
// El documento de transición lo nombra `BP_PlanetGravityHarness` asumiendo un Blueprint previo
// que en este repositorio nunca existió. Se implementa en C++ porque `AGENTS.md` §4 reserva los
// Blueprints para presentación y composición, no para lógica.
UCLASS()
class ASTRAEON_API AAstraeonPlanetGravityHarness : public AActor
{
	GENERATED_BODY()

public:
	AAstraeonPlanetGravityHarness();

	// Radio en centímetros. Es un **dato**: cambiarlo reescala la malla de prueba, pero el
	// runtime planetario definitivo no obtendrá su tamaño de ninguna escala de malla.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Planet")
	double PlanetRadiusCm = 20000.0;

	// Gravedad en la superficie. Sólo alimenta la magnitud; la dirección siempre es radial.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Planet")
	float SurfaceGravityMS2 = 9.81f;

	UFUNCTION(BlueprintPure, Category = "Astraeon|Planet")
	FVector GetPlanetCenterCm() const { return GetActorLocation(); }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Planet")
	double GetPlanetRadiusCm() const { return PlanetRadiusCm; }

	// Punto sobre la superficie en una dirección dada, con altura opcional. Lo usan el mapa de
	// prueba y las pruebas para colocar cosas sin tocar el suelo con un trace.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Planet")
	FVector GetSurfacePointCm(const FVector& DirectionFromCenter, double HeightAboveSurfaceCm = 0.0) const;

	// Devuelve el harness activo del mundo, o nullptr. Un mapa de prueba tiene exactamente uno;
	// si hubiera varios, el gameplay cercano debe usar un solo cuerpo gravitatorio (§4.9) y esta
	// función lo hace explícito devolviendo el primero y avisando.
	static AAstraeonPlanetGravityHarness* FindActiveHarness(const UWorld* World);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	// La malla se escala a partir de PlanetRadiusCm para que la esfera visible y el radio que usa
	// la gravedad no puedan divergir. Divergir aquí produciría un personaje flotando o hundido
	// sin ningún error visible.
	void ApplyRadiusToMesh();

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Planet")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Planet")
	TObjectPtr<UStaticMeshComponent> SphereMesh;
};
